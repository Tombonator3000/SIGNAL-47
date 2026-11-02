// Isolated Freeform route: ordinary autopilot physics through both real handoffs.
#include "S47StationWorldRoute.h"
#include "S47HighwayLandscape.h"
#include "S47StationPhone.h"
#include "S47Web.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Drive.h"
#include "S47Geo.h"
#include "S47Legs.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Json.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"
namespace
{
 struct FState
 {
  TWeakObjectPtr<AS47Web> W;int32 Stage=0,Checks=0,Changes=0,Waypoint=0;bool Done=false,Started=false,HavePosition=false;
  double Began=0,At=0,SampleAt=0,Distance=0,MaxStep=0;FVector Last=FVector::ZeroVector;
  FString Root,LastLeg;TArray<FVector2D> Walk;TArray<TSharedPtr<FJsonValue>> Evidence,Trace;
 };
 FState State;
 void Go(int32 Stage){State.Stage=Stage;State.At=FPlatformTime::Seconds();}
 void Finish(US47Main* M,bool Good,const FString& Why)
 {
  if(State.Done)return;State.Done=true;if(M&&M->Player)M->Player->TestMove.Reset();if(M&&M->World)M->World->Autopilot.Reset();
  auto R=S47J::New();S47J::Set(R,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(R,TEXT("reason"),Why);S47J::Set(R,TEXT("checks"),State.Checks);S47J::Set(R,TEXT("areaTransitions"),State.Changes);S47J::Set(R,TEXT("actualDrivenMetres"),State.Distance);S47J::Set(R,TEXT("maximumCanonicalDriveStep"),State.MaxStep);
  S47J::Set(R,TEXT("method"),TEXT("Fresh isolated Freeform developer case0. Ordinary production StartTrip/autopilot/Frame/Drive physics from SARO to Station01 and back, through the actual turn and 330/322m area handoffs. Ordinary Walker input through the walk gate and hut doorway, full JSON/Flow restore, then retraced walk and return drive. No Enter/Place handoff or destination fixture."));
  S47J::Set(R,TEXT("limits"),TEXT("Autopilot/Walker input driver, not human route finding. Continuous route photos require visual review for the reported black surfaces; the test does not declare unknown legacy geometry fixed."));R->SetArrayField(TEXT("evidence"),State.Evidence);R->SetArrayField(TEXT("routeTrace"),State.Trace);
  if(!State.Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(R,true),*(State.Root/TEXT("result.json")));
  UE_LOG(LogTemp,Display,TEXT("S47 StationWorld %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
 }
 bool Check(US47Main* M,bool Good,const FString& Text)
 {
  ++State.Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Text);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),State.Stage);State.Evidence.Add(MakeShared<FJsonValueObject>(O));
  UE_LOG(LogTemp,Display,TEXT("S47 StationWorld %s %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Text);if(!Good)Finish(M,false,Text);return Good;
 }
 void Photo(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(State.Root/(FString(Name)+TEXT(".png")),true,false);}
 bool Landscape(US47Main* M,AS47Web* W)
 {
  TArray<FString> Lines;const bool Good=W->AuditHighwayLandscape(Lines);auto O=S47J::New();S47J::Set(O,TEXT("area"),W->Area());S47J::Set(O,TEXT("landscapeAudit"),Good);S47J::Set(O,TEXT("lines"),FString::Join(Lines,TEXT("\n")));State.Evidence.Add(MakeShared<FJsonValueObject>(O));return Check(M,Good,TEXT("actual shared landscape has the correct transform and visibility in ")+W->Area());
 }
 bool Walk(US47Main* M)
 {
  if(State.Waypoint>=State.Walk.Num()){M->Player->TestMove.Reset();return true;}
  const FVector2D P(M->Player->Pos.X,M->Player->Pos.Z);const FVector2D D=State.Walk[State.Waypoint]-P;
  if(D.Size()<.18){++State.Waypoint;M->Player->TestMove.Reset();return false;}
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Y);M->Player->Pitch=-.06;M->Player->TestMove=FVector2D(0,-1);return false;
 }
 void Path(TArray<FVector2D> Points){State.Walk=MoveTemp(Points);State.Waypoint=0;}
}
namespace S47StationWorldRoute
{
 void Update(AS47Web* W)
 {
  if(!FParse::Param(FCommandLine::Get(),TEXT("S47StationWorldRoute"))||State.Done)return;
  US47Main* M=W?US47Main::Get(W):nullptr;if(!M||!M->World||!M->Player||!M->Flow||!M->Freeform||!M->Freeform->Active()||M->Mode!=US47Main::EMode::Play||M->Flow->bStarting||S47Startup::Blocking())return;
  auto* World=M->World.Get();const double Now=FPlatformTime::Seconds();
  if(!State.Started)
  {
   FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
   if(!User.StartsWith(TEXT("/tmp/"))||!M->Flow->bDevRun||M->Flow->CaseId!=0||!M->IsFreeform()){Finish(M,false,TEXT("temporary Freeform developer case0 required"));return;}
   if(World->bBusy)return;State.Started=true;State.W=W;State.Began=Now;State.Root=FPaths::ProjectSavedDir()/TEXT("StationWorldRoute");IFileManager::Get().MakeDirectory(*State.Root,true);
   M->UI->Close(true);if(M->Freeform->Director)M->Freeform->Director->Stop();
   if(!Check(M,World->Area==TEXT("saro"),TEXT("ordinary fresh SARO starting area"))||!Check(M,S47StationPhone::Ready(W)&&S47StationPhone::RemovedTriangles(W)>=20,TEXT("actual reused payphone installed and only its legacy triangles retired")))return;
   if(!Landscape(M,W))return;
   if(FParse::Param(FCommandLine::Get(),TEXT("S47StationWorldAssetsOnly"))){Finish(M,true,TEXT("cooked model and exact phone remainder ready; separate source run covers physical route"));return;}
   Photo(TEXT("saro-out"));World->StartTrip(TEXT("saro"),TEXT("station01"));World->Autopilot=13.;Go(1);return;
  }
  if(Now-State.Began>420){Finish(M,false,TEXT("bounded round-trip route timed out"));return;}
  if(World->bDriving)
  {
   if(auto* D=World->CabDrive())
   {
    const FVector Canon=W->HighwayMapEye(D->Pos);
    if(State.HavePosition){const double Step=FVector2D(Canon.X-State.Last.X,Canon.Z-State.Last.Z).Size();State.Distance+=Step;State.MaxStep=FMath::Max(State.MaxStep,Step);if(Step>12){Finish(M,false,TEXT("real drive coordinate handoff jumped more than12m"));return;}}
    State.Last=Canon;State.HavePosition=true;
    if(World->Leg!=State.LastLeg)
    {
     if(!State.LastLeg.IsEmpty()){++State.Changes;if(!Landscape(M,W))return;Photo(World->Leg==TEXT("station01")?TEXT("station-out-after"):World->Leg==TEXT("road")&&State.Stage>=5?TEXT("road-return-after"):TEXT("road-out-after"));}
     State.LastLeg=World->Leg;
    }
    if(Now-State.SampleAt>.2)
    {
     auto O=S47J::New();S47J::Set(O,TEXT("seconds"),Now-State.Began);S47J::Set(O,TEXT("leg"),World->Leg);S47J::Set(O,TEXT("canonicalPosition"),TArray<double>{Canon.X,Canon.Y,Canon.Z});S47J::Set(O,TEXT("speed"),D->Speed);State.Trace.Add(MakeShared<FJsonValueObject>(O));State.SampleAt=Now;
    }
   }
  }
  if(State.Stage==1)
  {
   if(World->bBusy||World->bDriving||Now-State.At<3)return;
   if(!Check(M,World->Area==TEXT("station01")&&State.Changes>=2,TEXT("actual outward route turns off highway and crosses both area handoffs")))return;
   int32 Parts=0;for(int32 I=0;;++I){const auto* N=W->Node(I);if(!N)break;if(N->Name.StartsWith(TEXT("SM_FieldStation_Hut_")))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))if(C->GetStaticMesh()&&C->GetStaticMesh()->GetNumLODs()==3&&C->IsVisible())++Parts;}
   bool Gate=false;for(const auto& Z:W->ZonesMutable(TEXT("station01")))if(Z.Id==TEXT("walkThrough"))Gate=Z.bEnabled;
   if(!Check(M,Parts==5&&Gate,TEXT("all five actual native hut parts and Freeform walk-through gate are available")))return;
   Photo(TEXT("station-yard"));Path({{-22.315,8029.},{-22.315,8023.4},{-22.315,8019.},{-10.05,8006.},{-10.05,8003.8}});Go(2);return;
  }
  if(State.Stage==2)
  {
   if(!Walk(M))return;Photo(TEXT("hut-inside"));
   if(!Check(M,M->Player->Pos.Z<8004.2,TEXT("ordinary Walker input crosses Freeform gate and actual hut doorway")))return;
   auto Save=M->Flow->GameState();const FString File=State.Root/TEXT("hut-continue.json");FString Text;
   if(!Check(M,FFileHelper::SaveStringToFile(S47J::Write(Save),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("full ordinary hut save written and freshly read")))return;
   US47Flow::FStart Start;Start.bDev=true;Start.State=S47J::Read(Text);M->Flow->StartGame(Start);State.HavePosition=false;Go(3);return;
  }
  if(State.Stage==3)
  {
   if(World->bBusy||Now-State.At<.6)return;bool Gate=false;for(const auto& Z:W->ZonesMutable(TEXT("station01")))if(Z.Id==TEXT("walkThrough"))Gate=Z.bEnabled;
   if(!Check(M,M->IsFreeform()&&World->Area==TEXT("station01")&&Gate&&M->Player->Pos.Z<8004.2,TEXT("disk/Flow continue retains hut position and reopens only the Freeform walk gate")))return;
   if(M->Freeform->Director)M->Freeform->Director->Stop();Path({{-10.05,8006.},{-22.315,8019.},{-22.315,8023.4},{-22.315,8029.},{-19.25,8031.4}});Go(4);return;
  }
  if(State.Stage==4)
  {
   if(!Walk(M))return;Photo(TEXT("station-return-before"));World->StartTrip(TEXT("station01"),TEXT("saro"));World->Autopilot=13.;State.HavePosition=false;Go(5);return;
  }
  if(State.Stage==5)
  {
   if(World->bBusy||World->bDriving||Now-State.At<3)return;
   if(!Check(M,World->Area==TEXT("saro")&&State.Changes>=4&&State.Distance>1800,TEXT("ordinary physical return drive crosses Station01/road/SARO with continuous canonical coordinates")))return;
   if(!Landscape(M,W))return;Photo(TEXT("saro-return"));Go(6);return;
  }
  if(State.Stage==6&&Now-State.At>.5)Finish(M,true,TEXT("actual Station01 return journey, gate/hut walking and disk continue complete; route PNGs await visual review"));
 }
}
