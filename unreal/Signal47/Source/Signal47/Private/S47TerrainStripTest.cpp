#include "S47TerrainStripTest.h"
#include "S47Web.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47RoadTerrain.h"
#include "S47RoadArea.h"
#include "S47Drive.h"
#include "S47Truck.h"
#include "S47Three.h"
#include "S47Json.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"
namespace
{
 struct FProbe {bool Started=false,Done=false,Motel=false,ReturnCab=false;int32 Stage=0;double Began=0,At=0;FString Root,ImageName;S47J::FObj Camera;TArray<TSharedPtr<FJsonValue>> Rays,Candidates;};FProbe P;
 void Photo(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(P.Root/(FString(Name)+TEXT(".png")),true,false);}
 void End(US47Main* M,bool Good,const FString& Why)
 {
  if(P.Done)return;P.Done=true;if(M&&M->Player)M->Player->CameraOverride=nullptr;
  auto J=S47J::New();S47J::Set(J,TEXT("status"),Good?TEXT("DIAGNOSTIC_CAPTURED"):TEXT("FAIL"));S47J::Set(J,TEXT("reason"),Why);
  S47J::Set(J,TEXT("method"),P.ReturnCab?TEXT("Measured first return-road pose reconstructed with production Drive/Ground/Pose and actual service DriverEye; cameraFixture records resulting eye and rotation. The truck is not being driven: ordinary visibility updates may hide the cab, and the observed capture contains terrain only. This does not reproduce the original cab/handoff frame. Settled suspension differs from moving spring/shake state. No geometry changes."):P.Motel?TEXT("Explicit isolated SARO motel sign-camera fixture (-24,1.9,635), yaw2.65/pitch.12; full-screen projected native bounds and individual component complex ray hit layers. Identifies actual original sign nodes versus motel-v2 parts; no asset/geometry changes or presumption that the large sign is motelBlade."):TEXT("Explicit isolated road-camera fixture near canonical7669,500, not a driven route. Actual native viewport rays and per-component complex collision traces; bounds-projection candidates include noncollision render geometry. No asset/geometry changes and no automatic claim of root cause."));
  if(P.Camera)J->SetObjectField(TEXT("cameraFixture"),P.Camera);J->SetArrayField(TEXT("rays"),P.Rays);J->SetArrayField(TEXT("projectedCandidates"),P.Candidates);
  FFileHelper::SaveStringToFile(S47J::Write(J,true),*(P.Root/TEXT("result.json")));UE_LOG(LogTemp,Display,TEXT("S47 TerrainStrip %s %s"),Good?TEXT("CAPTURED"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
 }
 TSharedPtr<FJsonObject> Describe(AS47Web* W,UStaticMeshComponent* C)
 {
  auto J=S47J::New();S47J::Set(J,TEXT("component"),C->GetPathName());UStaticMesh* Mesh=C->GetStaticMesh();S47J::Set(J,TEXT("mesh"),GetPathNameSafe(Mesh));S47J::Set(J,TEXT("boundsUE"),C->Bounds.GetBox().ToString());S47J::Set(J,TEXT("transform"),C->GetComponentTransform().ToHumanReadableString());
  S47J::Set(J,TEXT("visible"),C->IsVisible());S47J::Set(J,TEXT("hidden"),C->bHiddenInGame);S47J::Set(J,TEXT("collision"),int32(C->GetCollisionEnabled()));S47J::Set(J,TEXT("cpuAccess"),Mesh&&Mesh->bAllowCPUAccess);S47J::Set(J,TEXT("lods"),Mesh?Mesh->GetNumLODs():0);
  TArray<TSharedPtr<FJsonValue>> Materials;for(auto* Mat:C->GetMaterials()){auto R=S47J::New();S47J::Set(R,TEXT("asset"),GetPathNameSafe(Mat));S47J::Set(R,TEXT("twoSided"),Mat&&Mat->IsTwoSided());S47J::Set(R,TEXT("blend"),Mat?int32(Mat->GetBlendMode()):-1);Materials.Add(MakeShared<FJsonValueObject>(R));}J->SetArrayField(TEXT("materials"),Materials);
  for(int32 I=0;;++I){const auto* N=W->Node(I);if(!N)break;if(N->Comp.Get()==C){S47J::Set(J,TEXT("node"),I);S47J::Set(J,TEXT("geo"),N->Geo);S47J::Set(J,TEXT("name"),N->Name);S47J::Set(J,TEXT("parent"),N->Parent);break;}}
  return J;
 }
 void Measure(AS47Web* W,APlayerController* PC)
 {
  int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);TArray<UStaticMeshComponent*> Components;W->GetComponents(Components);
  // Screen band contains both reported black plates; use actual projection, not inferred world heights.
  for(int32 Y=0;Y<((P.Motel||P.ReturnCab)?9:5);++Y)for(int32 X=0;X<13;++X)
  {
   const double U=(P.Motel||P.ReturnCab)?.02+X*.08:.38+X*.05,V=(P.Motel||P.ReturnCab)?.02+Y*.12:.38+Y*.035;FVector Origin,Direction;if(!PC->DeprojectScreenPositionToWorld(U*Width,V*Height,Origin,Direction))continue;
   const FVector End=Origin+Direction*250000.;auto R=S47J::New();S47J::Set(R,TEXT("uv"),TArray<double>{U,V});S47J::Set(R,TEXT("originThree"),S47Three::ToThree(Origin).ToString());S47J::Set(R,TEXT("directionUE"),Direction.ToString());TArray<TSharedPtr<FJsonValue>> Hits;
   FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TerrainStrip),true);Q.bReturnFaceIndex=true;
   for(auto* C:Components)if(C&&C->IsVisible()&&!C->bHiddenInGame&&C->GetStaticMesh())
   {
    FHitResult H;if(!C->LineTraceComponent(H,Origin,End,Q))continue;auto A=Describe(W,C);S47J::Set(A,TEXT("distanceM"),H.Distance*.01);S47J::Set(A,TEXT("face"),H.FaceIndex);S47J::Set(A,TEXT("impactThree"),S47Three::ToThree(H.ImpactPoint).ToString());S47J::Set(A,TEXT("normalUE"),H.ImpactNormal.ToString());Hits.Add(MakeShared<FJsonValueObject>(A));
   }
   R->SetArrayField(TEXT("allComponentHits"),Hits);P.Rays.Add(MakeShared<FJsonValueObject>(R));
  }
  if(P.Motel)for(int32 I=0;;++I)
  {
   const auto* N=W->Node(I);if(!N)break;
   if((I>=26&&I<=28)||N->Name==TEXT("prop:motelBlade")||N->Name==TEXT("prop:motelNeon")||(N->Name.StartsWith(TEXT("motel-v2:"))&&N->Name.Contains(TEXT("Sign"))))
   if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
   {
    auto J=Describe(W,C);S47J::Set(J,TEXT("recordType"),TEXT("explicit sign inventory including hidden predecessors"));P.Candidates.Add(MakeShared<FJsonValueObject>(J));
    UE_LOG(LogTemp,Display,TEXT("S47 MotelSign node%d name=%s geo=%d mesh=%s visible=%d hidden=%d"),I,*N->Name,N->Geo,*GetPathNameSafe(C->GetStaticMesh()),C->IsVisible(),C->bHiddenInGame);
   }
  }
  for(auto* C:Components)if(C&&C->IsVisible()&&!C->bHiddenInGame&&C->GetStaticMesh())
  {
   const FBox B=C->Bounds.GetBox();double MinX=1e30,MinY=1e30,MaxX=-1e30,MaxY=-1e30;bool Any=false;
   for(int32 I=0;I<8;++I){FVector2D S;if(PC->ProjectWorldLocationToScreen(FVector(I&1?B.Max.X:B.Min.X,I&2?B.Max.Y:B.Min.Y,I&4?B.Max.Z:B.Min.Z),S)){Any=true;MinX=FMath::Min(MinX,S.X/Width);MaxX=FMath::Max(MaxX,S.X/Width);MinY=FMath::Min(MinY,S.Y/Height);MaxY=FMath::Max(MaxY,S.Y/Height);}}
   if(!Any||MaxX<((P.Motel||P.ReturnCab)?0.:.38)||MinX>1||MaxY<((P.Motel||P.ReturnCab)?0.:.38)||MinY>((P.Motel||P.ReturnCab)?1.:.53))continue;auto J=Describe(W,C);S47J::Set(J,TEXT("projectedBoundsUV"),TArray<double>{MinX,MinY,MaxX,MaxY});P.Candidates.Add(MakeShared<FJsonValueObject>(J));
  }
 }
}
namespace S47TerrainStripTest
{
 void Update(AS47Web* W)
 {
  if(P.Done||(!FParse::Param(FCommandLine::Get(),TEXT("S47TerrainStripTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("S47ProbeMotelSign"))&&!FParse::Param(FCommandLine::Get(),TEXT("S47ProbeReturnCab"))))return;auto* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->Flow||!M->World||!M->Player||!M->Freeform||!M->Freeform->Active()||M->Mode!=US47Main::EMode::Play||M->Flow->bStarting||S47Startup::Blocking())return;
  const double Now=FPlatformTime::Seconds();
  if(!P.Started)
  {
   FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);if(!User.StartsWith(TEXT("/tmp/"))||!M->Flow->bDevRun||M->Flow->CaseId!=0)return;
   P.Started=true;P.Motel=FParse::Param(FCommandLine::Get(),TEXT("S47ProbeMotelSign"));P.ReturnCab=FParse::Param(FCommandLine::Get(),TEXT("S47ProbeReturnCab"));P.ImageName=P.ReturnCab?TEXT("return-cab-fixture"):P.Motel?TEXT("motel-sign-fixture"):TEXT("road-fixture");P.At=P.Began=Now;P.Root=FPaths::ProjectSavedDir()/(P.ReturnCab?TEXT("ReturnCabProbe"):P.Motel?TEXT("MotelSignProbe"):TEXT("TerrainStripTest"));IFileManager::Get().MakeDirectory(*P.Root,true);M->UI->Close(true);if(M->Freeform->Director)M->Freeform->Director->Stop();M->World->Enter(P.Motel?TEXT("saro"):TEXT("road"));
   if(P.ReturnCab)
   {
    // Exact first road sample after the actual StationWorld return handoff; heading from adjacent trace points.
    const double X=7674.9063966089,Z=496.8104152327848;
    const double Heading=FMath::Atan2(-(7674.418889517647-7671.961792116477),-(496.8416537729534-497.00447963090613));
    M->World->PlaceTruck(TEXT("diner"));auto* Truck=M->World->TruckHere();if(!Truck){End(M,false,TEXT("actual road service truck unavailable"));return;}
    FS47RoadArea Area(FVector(8000,0,0));FS47DriveController Drive(Truck,&Area);Drive.bCameraMotion=M->Player->bHeadMotion;Drive.Place(FVector(X,0,Z),Heading);
    FS47View View;View.Eye=Drive.CamPos;double E[16];S47Three::Compose(FVector::ZeroVector,Drive.CamQuat,FVector::OneVector,E);
    View.Pitch=FMath::Asin(-FMath::Clamp(E[9],-1.,1.));View.Yaw=FMath::Atan2(E[8],E[10]);View.Roll=FMath::Atan2(E[1],E[5]);Truck->ViewFrom(View.Eye);
    P.Camera=S47J::New();S47J::Set(P.Camera,TEXT("method"),TEXT("Measured route first road return sample; production Drive::Place/Ground/Pose with actual service Truck DriverEye and RoadArea. Camera-math fixture only: bDriving remains false, ordinary visibility may hide the cabin, and native capture did not show it. Static settled suspension; not the dynamic handoff frame."));S47J::Set(P.Camera,TEXT("truckCenter"),TArray<double>{X,0,Z});S47J::Set(P.Camera,TEXT("driverEye"),Truck->DriverEye.ToString());S47J::Set(P.Camera,TEXT("actualEye"),View.Eye.ToString());S47J::Set(P.Camera,TEXT("quaternion"),Drive.CamQuat.ToString());S47J::Set(P.Camera,TEXT("heading"),Heading);
    M->Player->CameraOverride=[View](double,const FVector2D&,const FVector2D&,FS47View& Out){Out=View;return true;};return;
   }
   const double Y=S47RoadTerrain::SurfaceY(-331,500)+1.9;const bool Motel=P.Motel;M->Player->CameraOverride=[Y,Motel](double,const FVector2D&,const FVector2D&,FS47View& View){View.Eye=Motel?FVector(-24,1.9,635):FVector(7669,Y,500);View.Yaw=Motel?2.65:-UE_DOUBLE_HALF_PI;View.Pitch=Motel?.12:-.035;return true;};return;
  }
  if(Now-P.Began>35){End(M,false,TEXT("bounded diagnostic timed out"));return;}
  if(P.Stage==0&&Now-P.At>2){auto* PC=W->GetWorld()->GetFirstPlayerController();if(!PC){End(M,false,TEXT("native player controller unavailable"));return;}Measure(W,PC);Photo(*P.ImageName);P.Stage=1;P.At=Now;return;}
  if(P.Stage==1&&Now-P.At>1){const bool PNG=IFileManager::Get().FileSize(*(P.Root/(P.ImageName+TEXT(".png"))))>1000;End(M,PNG&&P.Candidates.Num()>0,TEXT("native horizon capture and component/material candidates recorded; root cause requires measured review"));}
 }
}
