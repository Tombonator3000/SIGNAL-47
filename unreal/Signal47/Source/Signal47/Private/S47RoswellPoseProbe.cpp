#include "S47RoswellPoseProbe.h"
#include "S47Glimpses.h"
#include "S47RoswellMotion.h"
#include "S47Flow.h"
#include "S47Flashlight.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Three.h"
#include "Components/PoseableMeshComponent.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Drives the real imported body and the production Motion::Apply path. It is
// an isolated deformation fixture, not a replacement AI or a player save.
struct FS47RoswellPoseDriver
{
 FString Dir; FS47RoswellMotion Motion; double T=0,Settle=0,Distance=0; int32 Shot=0;
 bool Started=false, Baseline=false, Pending=false; FString PendingPath;
 FVector At=FVector::ZeroVector;
 bool Step(US47Main* M,double Dt)
 {
  if (!M || !M->Web || !M->Web->IsLoaded() || !M->Flow || !M->Night || !M->World || !M->Player ||
   S47Startup::Blocking() || M->Flow->bStarting || M->World->bBusy || M->Mode!=US47Main::EMode::Play) return true;
  auto& G=*M->Night->Glimpses;
  if (Pending)
  {
   if (!IFileManager::Get().FileExists(*PendingPath)) return true;
   Pending=false;
   if ((Baseline && Shot>=1) || Shot>=10)
   {UE_LOG(LogTemp,Display,TEXT("S47: Roswell pose COMPLETE images=%d baseline=%d"),Shot,Baseline?1:0);G.bPreviewDriver=false;G.Hide();return false;}
   Settle=0;Motion.Reset();Distance=0;
  }
  if (!Started)
  {
   FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
   if (!M->Flow->bDevRun || M->Flow->CaseId!=0 || !User.StartsWith(TEXT("/tmp/")) ||
    !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(User)) || !G.HasNativeBody())
   { UE_LOG(LogTemp,Error,TEXT("S47: Roswell pose FAIL: requires isolated native Case0 /tmp profile")); return false; }
   G.DevClear();G.bPreviewDriver=true;
   At=FVector(10.,US47Night::SaroFloorAt(M->Web.Get(),10.,-9.),-9.);
   M->Player->Place(6.95,-7.8,0); M->Player->Pitch=-.16;
   if (M->Night->Flashlight) {M->Night->Flashlight->bHave=true;M->Night->Flashlight->bOn=false;}
   M->UI->ClearToasts();IFileManager::Get().MakeDirectory(*Dir,true);Started=true;
   if (auto* C=G.Body.Mesh.Get()) for (const TCHAR* Name:{TEXT("spine_03"),TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r")})
   {
    const int32 B=G.Body.Names.IndexOfByKey(FName(Name));FTransform P=G.Body.Rest[B];
    for (int32 A=G.Body.Parents[B];A>=0;A=G.Body.Parents[A]) P=P*G.Body.Rest[A];
    UE_LOG(LogTemp,Display,TEXT("S47: Roswell rest bone=%s nativeCm=%s"),Name,*P.GetLocation().ToCompactString());
   }
  }
  Dt=FMath::Clamp(Dt,0.,.05);T+=Dt;Settle+=Dt;
  const int32 Pose=Baseline?0:Shot/2;
  const bool Side=!Baseline && Shot%2;
  const double Speed=Pose==2?.65:Pose==3?2.8:0.; Distance+=Speed*Dt;
  const double Yaw=Side?PI*.5:PI;
  const FVector DrawAt=At+FVector(FMath::Sin(Yaw),0,FMath::Cos(Yaw))*Speed*Settle;
  M->Player->Place(DrawAt.X,DrawAt.Z-2.55,0);
  if (auto* Root=G.BodyRoot.Get())
  {Root->SetWorldLocation(S47Three::ToUE(DrawAt));Root->SetWorldRotation(FRotationMatrix::MakeFromX(S47Three::DirToUE(FVector(FMath::Sin(Yaw),0,FMath::Cos(Yaw)))).Rotator());}
  FS47RoswellPose P;P.Mode=Pose==1?ES47RoswellMotion::Listen:Pose==2?ES47RoswellMotion::Stalk:Pose==3?ES47RoswellMotion::Run:Pose==4?ES47RoswellMotion::Grab:ES47RoswellMotion::Idle;
  P.Crouch=Pose==2?.65:Pose==1?.22:0.;P.SpeedMps=Speed;P.DistanceTravelled=Distance;
  Motion.Apply(G.Body,P,Dt);
  const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z), D=DrawAt+FVector(0,.65,0)-Eye;
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z); M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  if (Settle<1.3) return true;
  auto* C=G.Body.Mesh.Get(); bool Clear=true;
  const FVector Chest=C->GetBoneLocationByName(TEXT("spine_03"),EBoneSpaces::ComponentSpace);
  for (int32 S=0;S<2;++S)
  {
   const FVector Shoulder=C->GetBoneLocationByName(S==0?TEXT("upperarm_l"):TEXT("upperarm_r"),EBoneSpaces::ComponentSpace);
   const FVector Elbow=C->GetBoneLocationByName(S==0?TEXT("lowerarm_l"):TEXT("lowerarm_r"),EBoneSpaces::ComponentSpace);
   const FVector Hand=C->GetBoneLocationByName(S==0?TEXT("hand_l"):TEXT("hand_r"),EBoneSpaces::ComponentSpace);
   const double Sign=FMath::Sign(Shoulder.X-Chest.X), Gap=FMath::Min(Sign*(Elbow.X-Chest.X),Sign*(Hand.X-Chest.X))-FMath::Abs(Shoulder.X-Chest.X);
   Clear&=Gap>3.;
   UE_LOG(LogTemp,Display,TEXT("S47: Roswell pose=%d side=%d limb=%d shoulder=%s elbow=%s hand=%s outwardGap=%.3f"),Pose,Side?1:0,S,*Shoulder.ToCompactString(),*Elbow.ToCompactString(),*Hand.ToCompactString(),Gap);
  }
  PendingPath=Dir/FString::Printf(TEXT("pose-%02d.png"),Shot);
  FScreenshotRequest::RequestScreenshot(PendingPath,true,false);Pending=true;
  if (!Baseline && !Clear) UE_LOG(LogTemp,Error,TEXT("S47: Roswell pose FAIL: elbow/wrist clearance pose=%d"),Pose);
  ++Shot;
  return true;
 }
};
void S47RoswellPoseProbe::Start(US47Main* M)
{
 FString Dir;if (!M || !FParse::Value(FCommandLine::Get(),TEXT("S47RoswellPoseShots="),Dir)) return;
 const auto P=MakeShared<FS47RoswellPoseDriver>();P->Dir=Dir;P->Baseline=FParse::Param(FCommandLine::Get(),TEXT("S47RoswellPoseBaseline"));const TWeakObjectPtr<US47Main> Weak=M;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,P](float Dt){return Weak.IsValid() && P->Step(Weak.Get(),Dt);}));
}
