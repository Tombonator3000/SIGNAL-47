#include "S47FloodProbe.h"
#include "S47CalicheAccess.h"
#include "S47FarmHouse.h"
#include "S47Main.h"
#include "S47OutsideLight.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "DynamicRHI.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FloodProbe,Log,All);

namespace
{
 // Three's, with Y over the room's floor. Eye and where it looks; the source out in the open and what it is aimed at.
 struct FEyeAt{const TCHAR* Name;FVector Eye,Look;};
 struct FFloodRoom
 {
  const TCHAR* Name;const TCHAR* Area;const TCHAR* Say;
  FVector2D Stand;FVector From,Aim;FEyeAt Views[4];
 };
 // The windows in question: the control room's two in its north wall; the old house's in its north wall (plan x 1.6..3.4, S47FarmHouse);
 // the garage's roll-up door in its south wall, x 33.2..37.6; the cinema's three glazed bays on the street, under the marquee.
 const FFloodRoom Rooms[]={
  {TEXT("control"),TEXT("saro"),TEXT("SARO control room"),FVector2D(3.6,3.3),FVector(-1.5,9.5,-25),FVector(-1.5,1,-1),{
   {TEXT("A"),FVector(3.6,1.62,3.3),FVector(-2,1.5,-4.2)},
   {TEXT("B"),FVector(-5.1,1.62,.8),FVector(5,1.1,-1)},
   {TEXT("C"),FVector(.5,2,2.6),FVector(-1,0,-2.2)},
   {TEXT("D"),FVector(-12,2.2,-14),FVector(-2,1.5,-4)}}},
  {TEXT("house"),TEXT("saro"),TEXT("the old house"),FVector2D(75.4,521),FVector(77.5,7,542),FVector(77.5,1,522),{
   {TEXT("A"),FVector(75.4,1.62,521),FVector(77.5,1.5,525)},
   {TEXT("B"),FVector(79.4,1.62,522.4),FVector(75.1,1.1,522.8)},
   {TEXT("C"),FVector(76.5,2,521.4),FVector(77.6,0,523.6)},
   {TEXT("D"),FVector(83,2,534),FVector(76,1.5,525)}}},
  {TEXT("garage"),TEXT("saro"),TEXT("the garage"),FVector2D(33.2,-4),FVector(35.4,8.5,30),FVector(35.4,1,0),{
   {TEXT("A"),FVector(33.2,1.62,-4),FVector(35.4,1.6,5.5)},
   {TEXT("B"),FVector(32.2,1.62,.5),FVector(38,1,.5)},
   {TEXT("C"),FVector(37.4,2.2,-1.5),FVector(35,0,2.5)},
   {TEXT("D"),FVector(30,2,18),FVector(35.4,1.5,5)}}},
  {TEXT("cinema"),TEXT("diner"),TEXT("the Plaza cinema"),FVector2D(7980.4,2158),FVector(8013,8,2161),FVector(7982,1,2161),{
   {TEXT("A"),FVector(7980.2,1.62,2158),FVector(7985,1.4,2160)},
   {TEXT("B"),FVector(7982,1.62,2170.5),FVector(7982,1,2154)},
   {TEXT("C"),FVector(7981,2.2,2161),FVector(7983,0,2160)},
   {TEXT("D"),FVector(8000,2,2145),FVector(7985,2,2160)}}},
 };
 // The rooms' own air with the lamps on, from out in the yard, for T16-13. Y over the ground, which is near 0 here.
 const FEyeAt HazeViews[]={
  {TEXT("control-north"),FVector(-1.8,1.7,-13),FVector(-1.8,1.8,-4.2)},
  {TEXT("control-east"),FVector(14,1.7,-9),FVector(2,1.8,-3)},
  {TEXT("garage-south"),FVector(35.4,1.7,17),FVector(35.4,1.6,4)},
 };
 constexpr double SettleSeconds=6,AreaSeconds=4,OffSeconds=2.5,LiveSeconds=40;
 // The pictures of the room, seconds after the light came on: three of one view over time, then the others.
 struct FShotAt{int32 View;double At;const TCHAR* Name;};
 const FShotAt Shots[]={{0,1.5,TEXT("A1")},{0,5,TEXT("A2")},{0,9,TEXT("A3")},{1,11.5,TEXT("B")},{2,14,TEXT("C")},{3,17,TEXT("D")}};
 const FLinearColor ProbeColor(.86f,.9f,1.f);

 struct FFloodRun
 {
  enum class EMode:uint8{None,Test,Live} Mode=EMode::None;
  enum class EPhase:uint8{Haze,RoomEnter,RoomSettle,RoomOff,RoomOn,Finish} Phase=EPhase::Haze;
  TWeakObjectPtr<US47Main> Main;
  TSharedPtr<FS47OutsideLight> Rig;
  TArray<int32> Queue;
  int32 At=0,HazeAt=0,ShotAt=0;
  double Clock=0,Since=0,LastFrame=0;
  double FrameSum=0,FrameMost=0,GpuSum=0;int32 FrameCount=0;
  double OffFrame=0,OffGpu=0;
  FEyeAt Now{TEXT(""),FVector::ZeroVector,FVector::ZeroVector};
  double Floor=0;
  FVector LiveFrom=FVector::ZeroVector;
  bool bLiveWalk=false;
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Before;
  bool bHooked=false;
 };
 FFloodRun Run;

 const FFloodRoom* RoomNamed(const FString& Name)
 {
  for(const FFloodRoom& R:Rooms)if(Name==R.Name)return &R;
  return nullptr;
 }
 double FloorOf(const FFloodRoom& R)
 {
  if(FCString::Strcmp(R.Name,TEXT("house"))==0)return S47FarmHouse::Point(0,0).Y;
  FVector Middle;
  if(FCString::Strcmp(R.Name,TEXT("cinema"))==0&&S47CalicheAccess::RoomMiddle(TEXT("Cinema"),Middle))return Middle.Y;
  return 0;
 }
 void Look(US47Main* M,const FEyeAt& V,double Floor)
 {
  Run.Now=V;Run.Floor=Floor;
  if(Run.bHooked||!M->Player)return;
  Run.Before=M->Player->CameraOverride;Run.bHooked=true;
  M->Player->CameraOverride=[](double,const FVector2D&,const FVector2D&,FS47View& Out)
  {
   const FVector Up(0,Run.Floor,0),Eye=Run.Now.Eye+Up,D=Run.Now.Look+Up-Eye;
   Out.Eye=Eye;Out.Yaw=FMath::Atan2(-D.X,-D.Z);Out.Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Out.Roll=0;
   return true;
  };
 }
 void Unhook(US47Main* M)
 {
  if(Run.bHooked&&M&&M->Player)M->Player->CameraOverride=Run.Before;
  Run.Before=nullptr;Run.bHooked=false;
 }
 void Shot(const FString& Name)
 {
  // The world only: this is about the picture, and the HUD would sit on the top of it.
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("AlbumTest")/Name,false,false);
  UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe: picture %s, %.1f s in"),*Name,Run.Since);
 }
 void Measure()
 {
  const double Now=FPlatformTime::Seconds();
  if(Run.LastFrame>0)
  {
   const double Ms=(Now-Run.LastFrame)*1000.;
   Run.FrameSum+=Ms;Run.FrameMost=FMath::Max(Run.FrameMost,Ms);Run.GpuSum+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());++Run.FrameCount;
  }
  Run.LastFrame=Now;
 }
 void Report(const FString& What,bool bKeepAsOff=false)
 {
  const double Frame=Run.FrameCount?Run.FrameSum/Run.FrameCount:0,Gpu=Run.FrameCount?Run.GpuSum/Run.FrameCount:0;
  if(bKeepAsOff){Run.OffFrame=Frame;Run.OffGpu=Gpu;}
  UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe: %s: frame %.1f ms on average, %.1f at worst, gpu %.1f ms, %d frames; over light off %+.1f frame, %+.1f gpu"),*What,Frame,Run.FrameMost,Gpu,Run.FrameCount,Frame-Run.OffFrame,Gpu-Run.OffGpu);
  Run.FrameSum=Run.FrameMost=Run.GpuSum=0;Run.FrameCount=0;
 }
 void LightFrame(const FFloodRoom& R,double Seconds,double K)
 {
  if(!Run.Rig)return;
  const FVector Up(0,Run.Floor,0);
  FS47OutsideLight::FFrame F;
  F.Aim=R.Aim+Up;F.From=FS47OutsideLight::Swept(R.From+Up,F.Aim,Seconds);F.Color=ProbeColor;F.K=K;F.Seconds=Seconds;
  Run.Rig->Show(F);
  Run.Rig->SetAir(FMath::Min(1.,Seconds/.8),R.Aim+Up+FVector(0,.1,0),FLinearColor(.9f,.9f,.92f),true);
 }
 void EndTest(US47Main* M)
 {
  Unhook(M);
  if(Run.Rig){Run.Rig->SetAir(0,FVector::ZeroVector,FLinearColor::White,true);Run.Rig->Stop();Run.Rig.Reset();}
  Run.Mode=FFloodRun::EMode::None;
 }
 void Enter(US47Main* M,const FFloodRoom& R)
 {
  if(M->Web->Area()!=R.Area&&M->World)
  {
   M->World->PlaceTruck(R.Area);M->World->Enter(R.Area);
   Run.Since=-AreaSeconds;
  }
  else Run.Since=0;
  UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe: going to %s"),R.Say);
 }
 bool StepTest(US47Main* M,float Dt)
 {
  Measure();
  Run.Since+=Dt;
  switch(Run.Phase)
  {
  case FFloodRun::EPhase::Haze:
  {
   if(Run.HazeAt>=int32(UE_ARRAY_COUNT(HazeViews))){Run.Phase=FFloodRun::EPhase::RoomEnter;return true;}
   const FEyeAt& V=HazeViews[Run.HazeAt];
   if(Run.Since<=Dt)
   {
    // Standing there as well: the rooms' haze goes by where the player is.
    M->Player->Place(V.Eye.X,V.Eye.Z,0);Look(M,V,0);
    Run.Since=Dt;
    return true;
   }
   if(Run.Since<SettleSeconds)return true;
   Shot(FString::Printf(TEXT("flood-haze-%s.png"),V.Name));
   Report(FString::Printf(TEXT("haze %s"),V.Name),true);
   ++Run.HazeAt;Run.Since=0;
   return true;
  }
  case FFloodRun::EPhase::RoomEnter:
  {
   if(Run.At>=Run.Queue.Num()){Run.Phase=FFloodRun::EPhase::Finish;return true;}
   const FFloodRoom& R=Rooms[Run.Queue[Run.At]];
   Enter(M,R);
   Run.Phase=FFloodRun::EPhase::RoomSettle;
   return true;
  }
  case FFloodRun::EPhase::RoomSettle:
  {
   if(Run.Since<0)return true;
   const FFloodRoom& R=Rooms[Run.Queue[Run.At]];
   const double Floor=FloorOf(R);
   M->Player->Place(R.Stand.X,R.Stand.Y,0);Look(M,R.Views[0],Floor);
   UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe: %s, floor %.2f, the walker's floor %.2f, area %s"),R.Name,Floor,M->Player->FloorY,*M->Web->Area());
   Run.Since=0;Run.FrameSum=Run.FrameMost=Run.GpuSum=0;Run.FrameCount=0;
   Run.Phase=FFloodRun::EPhase::RoomOff;
   return true;
  }
  case FFloodRun::EPhase::RoomOff:
  {
   const FFloodRoom& R=Rooms[Run.Queue[Run.At]];
   if(Run.Since<SettleSeconds){Run.FrameSum=Run.FrameMost=Run.GpuSum=0;Run.FrameCount=0;return true;}
   if(Run.Since<SettleSeconds+OffSeconds)return true;
   Shot(FString::Printf(TEXT("flood-%s-0off.png"),R.Name));
   Report(FString::Printf(TEXT("%s, the light off"),R.Name),true);
   Run.Since=0;Run.ShotAt=0;
   Run.Phase=FFloodRun::EPhase::RoomOn;
   return true;
  }
  case FFloodRun::EPhase::RoomOn:
  {
   const FFloodRoom& R=Rooms[Run.Queue[Run.At]];
   LightFrame(R,Run.Since,FMath::Min(1.,Run.Since/.25));
   if(Run.ShotAt>=int32(UE_ARRAY_COUNT(Shots)))
   {
    Run.Rig->Hide();Run.Rig->SetAir(0,FVector::ZeroVector,FLinearColor::White,true);
    ++Run.At;Run.Since=0;
    Run.Phase=FFloodRun::EPhase::RoomEnter;
    return true;
   }
   const FShotAt& S=Shots[Run.ShotAt];
   // Round to the next view a good while before its picture, never on the same frame as one.
   if(Run.Now.Eye!=R.Views[S.View].Eye&&Run.Since>S.At-2.2){Look(M,R.Views[S.View],Run.Floor);Run.FrameSum=Run.FrameMost=Run.GpuSum=0;Run.FrameCount=0;}
   if(Run.Since<S.At)return true;
   Shot(FString::Printf(TEXT("flood-%s-%s.png"),R.Name,S.Name));
   Report(FString::Printf(TEXT("%s %s, the light on"),R.Name,S.Name));
   ++Run.ShotAt;
   return true;
  }
  default:break;
  }
  UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe PASS: pictures taken in %d rooms; the pictures are what counts"),Run.Queue.Num());
  EndTest(M);
  FPlatformMisc::RequestExitWithStatus(false,0);
  return false;
 }
 bool StepLive(US47Main* M,float Dt)
 {
  if(!M->Player||!M->Web){EndTest(M);return false;}
  if(M->bPausedByMenu||M->ModalOpen())return true;
  Run.Since+=Dt;
  if(Run.Since<0)return true;
  if(Run.bLiveWalk&&Run.At>=0)
  {
   const FFloodRoom& R=Rooms[Run.At];
   M->Player->Place(R.Stand.X,R.Stand.Y,M->Player->Yaw);
   Run.Floor=FloorOf(R);Run.LiveFrom=R.From+FVector(0,Run.Floor,0);
   Run.bLiveWalk=false;Run.Since=0;
  }
  if(Run.Since>=LiveSeconds){EndTest(M);return false;}
  const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
  FS47OutsideLight::FFrame F;
  F.Aim=Feet+FVector(0,1.25,0);F.From=FS47OutsideLight::Swept(Run.LiveFrom,F.Aim,Run.Since);F.Color=ProbeColor;F.Seconds=Run.Since;
  F.K=FMath::Min(1.,Run.Since/.25)*(1-FMath::SmoothStep(LiveSeconds-1.2,LiveSeconds,Run.Since));
  Run.Rig->Show(F);
  Run.Rig->SetAir(FMath::Min(1.,Run.Since/.8),Feet+FVector(0,1.1,0),FLinearColor(.9f,.9f,.92f),true);
  return true;
 }
}

namespace S47FloodProbe
{
 void BeginTest(US47Main* Main,const FString& OnlyRoom)
 {
  EndTest(Main);
  Run=FFloodRun();
  Run.Mode=FFloodRun::EMode::Test;Run.Main=Main;Run.Rig=MakeShared<FS47OutsideLight>(Main);
  for(int32 I=0;I<int32(UE_ARRAY_COUNT(Rooms));++I)if(OnlyRoom.IsEmpty()||OnlyRoom==Rooms[I].Name)Run.Queue.Add(I);
  Run.Phase=FFloodRun::EPhase::Haze;
  if(!OnlyRoom.IsEmpty())Run.HazeAt=int32(UE_ARRAY_COUNT(HazeViews));
  UE_LOG(LogS47FloodProbe,Display,TEXT("S47: flood probe: %d rooms"),Run.Queue.Num());
 }
 bool BeginLive(US47Main* Main,const FString& Room,FString& OutWhy)
 {
  if(!Main||!Main->Player||!Main->Web){OutWhy=TEXT("no world");return false;}
  EndTest(Main);
  Run=FFloodRun();
  Run.Mode=FFloodRun::EMode::Live;Run.Main=Main;Run.Rig=MakeShared<FS47OutsideLight>(Main);Run.At=INDEX_NONE;
  if(const FFloodRoom* R=RoomNamed(Room))
  {
   Run.At=int32(R-Rooms);Run.bLiveWalk=true;
   Enter(Main,*R);
  }
  else
  {
   const FVector Chest(Main->Player->Pos.X,Main->Player->FloorY+1.25,Main->Player->Pos.Z);
   if(!FS47OutsideLight::FindWayIn(Main,Chest,Run.LiveFrom))
   {
    OutWhy=TEXT("no window or door lets a light reach you here");
    EndTest(Main);
    return false;
   }
  }
  return true;
 }
 void StopLive(US47Main* Main){if(Run.Mode==FFloodRun::EMode::Live)EndTest(Main);}
 bool LiveActive(){return Run.Mode==FFloodRun::EMode::Live;}
 bool Step(US47Main* Main,float Dt)
 {
  if(!Main||Dt<=0)return Run.Mode!=FFloodRun::EMode::None;
  if(Run.Mode==FFloodRun::EMode::Test)return StepTest(Main,Dt);
  if(Run.Mode==FFloodRun::EMode::Live)return StepLive(Main,Dt);
  return false;
 }
}
