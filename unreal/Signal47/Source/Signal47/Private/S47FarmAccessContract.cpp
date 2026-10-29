// Opt-in ENV04 evidence. One declared commute-start fixture, then ordinary World/Drive/Walker.
// Missing scenery/collider metadata never prevents the physical baseline from reaching the gate.
#include "S47Web.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StaticMeshResources.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FarmAccess,Log,All);
namespace
{
 using FJ=S47J::FObj;
 // These are Drive.cpp's actual collision half-extents, not the parked Walker proxy's larger box.
 constexpr double HalfWidth=1.,HalfLength=2.65,GateX=21.;
 const FVector SaroToRoad(24,0,-300);
 double GateZ(){return S47RoadTerrain::RanchZ(GateX);}
 double Angle(double A){return FMath::Atan2(FMath::Sin(A),FMath::Cos(A));}
 void Vec(const FJ& O,const TCHAR* Key,const FVector& V){S47J::Set(O,Key,TArray<double>{V.X,V.Y,V.Z});}
 bool JsonFile(const FString& File,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*File);}
 bool SameJson(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 FVector RoadPoint(const FVector& P,const FString& Leg){return Leg==TEXT("saro")?P+SaroToRoad:P-FVector(8000,0,0);}
 FVector ScenePoint(const FVector& P,const FString& Leg){return Leg==TEXT("saro")?P-SaroToRoad:P+FVector(8000,0,0);}
 FJ Transform(const FTransform& T)
 {
  auto O=S47J::New();Vec(O,TEXT("translation_ue_cm"),T.GetLocation());Vec(O,TEXT("scale"),T.GetScale3D());
  const FQuat Q=T.GetRotation();S47J::Set(O,TEXT("rotation_xyzw"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});return O;
 }
 bool PngReceipt(const FString& File,FJ& Out)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*File))return false;const uint8 Magic[]={137,80,78,71,13,10,26,10};
  if(B.Num()<33||FMemory::Memcmp(B.GetData(),Magic,8))return false;
  auto U32=[&](int64 I){return (uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;int32 Chunks=0;uint32 Width=0,Height=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>B.Num()-At-12)return false;const uint8* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(Chunks==0){if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13)return false;Width=U32(At+8);Height=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Idat+=Size;++Chunks;At+=int64(Size)+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=B.Num()||Idat<=0||Width<320||Height<200)return false;
  FSHAHash Hash;FSHA1::HashBuffer(B.GetData(),B.Num(),Hash.Hash);Out=S47J::New();
  S47J::Set(Out,TEXT("file"),File);S47J::Set(Out,TEXT("sha1"),Hash.ToString());S47J::Set(Out,TEXT("bytes"),B.Num());
  S47J::Set(Out,TEXT("width"),double(Width));S47J::Set(Out,TEXT("height"),double(Height));
  S47J::Set(Out,TEXT("all_chunk_crc_verified"),true);S47J::Set(Out,TEXT("iend_at_eof"),true);return true;
 }
 TArray<FVector> Footprint(const FVector& P,double H)
 {
  const FVector F(-FMath::Sin(H),0,-FMath::Cos(H)),R(FMath::Cos(H),0,-FMath::Sin(H));
  return {P+F*HalfLength+R*HalfWidth,P-F*HalfLength+R*HalfWidth,P-F*HalfLength-R*HalfWidth,P+F*HalfLength-R*HalfWidth};
 }
 bool AcrossGate(const TArray<FVector>& Polygon)
 {
  // Clip the measured oriented footprint to the far side of the actual gate plane.
  TArray<FVector> Beyond;for(int32 I=0;I<Polygon.Num();++I)
  {
   const FVector A=Polygon[I],B=Polygon[(I+1)%Polygon.Num()];const bool Ai=A.X>GateX+.015,Bi=B.X>GateX+.015;
   if(Ai)Beyond.Add(A);if(Ai!=Bi)Beyond.Add(FMath::Lerp(A,B,(GateX+.015-A.X)/(B.X-A.X)));
  }
  if(Beyond.IsEmpty())return false;double Z0=1e30,Z1=-1e30;for(const auto& P:Beyond){Z0=FMath::Min(Z0,P.Z);Z1=FMath::Max(Z1,P.Z);}
  return Z1>GateZ()-3.25&&Z0<GateZ()+3.2;
 }

 struct FControl : TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root,PendingFailure,PhotoName,SaveId;
  bool Started=false,Done=false,Tracking=false,PathDone=false,Reverse=false,PhotoRequested=false,CarBlocked=false,PersonBlocked=false;
  int32 Stage=0,Checks=0,Transitions=0,Cursor=0,WalkCursor=0,LastSaroRanchTriangles=0;
  double Began=FPlatformTime::Seconds(),At=Began,Last=Began,Ready=0,PhotoAt=0,ProgressAt=Began,StalledAt=0,LastTrace=0;
  double DriveMetres=0,WalkMetres=0,MaxSpeed=6,LastSpeed=0,WalkLast=0;
  FVector LastRoad=FVector::ZeroVector,ProgressRoad=FVector::ZeroVector,LastFoot=FVector::ZeroVector,EgressRoad=FVector::ZeroVector,ParkedRoad=FVector::ZeroVector;
  double ParkedHeading=0,SavedYaw=0,SavedPitch=0,SavedClock=0;FVector SavedFoot=FVector::ZeroVector;
  FString LastLeg;TArray<FVector> Path,Walk;TArray<TSharedPtr<FJsonValue>> Photos,Snapshots,Trace,Events;
  FJ PhotoMeta,BeforeSave,PackBefore;TMap<FString,bool> SavedDoors;

  US47Main* Main()const{return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  FString Leg()const{auto* M=Main();return M&&M->World?(M->World->Leg.IsEmpty()?(M->World->Area==TEXT("saro")?TEXT("saro"):TEXT("road")):M->World->Leg):FString();}
  FVector CarRoad()const{auto* M=Main();auto* D=M&&M->World?M->World->CabDrive():nullptr;return D?RoadPoint(D->Pos,Leg()):ParkedRoad;}
  void Brake()
  {
   auto* M=Main();if(!M||!M->World)return;M->World->Autopilot.Reset();TWeakObjectPtr<US47World> W=M->World;
   M->World->TestInput=[W]{FS47DriveIn I;if(auto* A=W.Get()){const double V=A->DrivingSpeed();I.Throttle=V>.035?-1:V<-.035?1:0;}return I;};
  }
  void Result(bool Pass,const FString& Why)
  {
   const TSharedRef<FControl> KeepAlive=AsShared();if(Done)return;Done=true;auto* M=Main();
   if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(M&&M->World){M->World->Autopilot.Reset();M->World->TestInput=nullptr;}
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("drive_metres"),DriveMetres);S47J::Set(O,TEXT("walk_metres"),WalkMetres);S47J::Set(O,TEXT("ordinary_leg_transitions"),Transitions);
   S47J::Set(O,TEXT("car_physically_blocked"),CarBlocked);S47J::Set(O,TEXT("person_physically_blocked"),PersonBlocked);
   S47J::Set(O,TEXT("last_saro_visible_ranch_triangles_in_expected_place"),LastSaroRanchTriangles);
   S47J::Set(O,TEXT("scope"),TEXT("One explicit BeginCommute road-Z535 start fixture, then ordinary World.Frame/Drive/TestInput/Walker; physical gate approach before metadata acceptance; actual scene/camera/viewport evidence; full Flow title/Continue only if earlier stages pass."));
   S47J::Set(O,TEXT("limits"),TEXT("API control, not human input or visual approval. No product geometry, collision, visibility, story or quest mutation. Original E01 camera/build identity remains unknown. A FAIL does not accept later unexecuted route stages."));
   O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("snapshots"),Snapshots);O->SetArrayField(TEXT("route_samples"),Trace);O->SetArrayField(TEXT("events"),Events);
   const bool Written=!Root.IsEmpty()&&JsonFile(Root/TEXT("result.json"),O);if(!Written)Pass=false;
   if(auto* W=Web.Get())W->GetWorldTimerManager().ClearTimer(Timer);
   if(Pass){UE_LOG(LogS47FarmAccess,Display,TEXT("S47: FarmAccess contract PASS drive=%.2fm walk=%.2fm checks=%d evidence=%s; human input/visual approval UNVERIFIED"),DriveMetres,WalkMetres,Checks,*Root);FPlatformMisc::RequestExit(false);}
   else{UE_LOG(LogS47FarmAccess,Error,TEXT("S47: FarmAccess contract FAIL stage=%d: %s; evidence=%s"),Stage,Written?*Why:TEXT("evidence write failed"),*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Good,const FString& Why){++Checks;if(!Good){Result(false,Why);return false;}UE_LOG(LogS47FarmAccess,Display,TEXT("S47: FarmAccess check PASS: %s"),*Why);return true;}
  void Next(int32 N,const TCHAR* Why)
  {
   Stage=N;At=FPlatformTime::Seconds();StalledAt=0;auto O=S47J::New();S47J::Set(O,TEXT("stage"),N);S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("at_seconds"),At-Began);Events.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogS47FarmAccess,Display,TEXT("S47: FarmAccess stage %d: %s"),N,Why);
  }
  FJ Pose()
  {
   auto O=S47J::New();auto* M=Main();S47J::Set(O,TEXT("leg"),M->World->Leg);S47J::Set(O,TEXT("world_area"),M->World->Area);S47J::Set(O,TEXT("web_area"),Web->Area());
   S47J::Set(O,TEXT("driving"),M->World->bDriving);S47J::Set(O,TEXT("truck_at"),M->World->TruckAt);S47J::Set(O,TEXT("personal_commute_vehicle"),M->World->bCommuteVehicle);
   if(auto* D=M->World->CabDrive())
   {
    Vec(O,TEXT("centre_three_m"),D->Pos);Vec(O,TEXT("centre_road_local_m"),RoadPoint(D->Pos,Leg()));S47J::Set(O,TEXT("heading_rad"),D->Heading);S47J::Set(O,TEXT("speed_m_s"),D->Speed);
    TArray<TSharedPtr<FJsonValue>> Corners;double MaxX=-1e30;for(const auto& P:Footprint(RoadPoint(D->Pos,Leg()),D->Heading)){auto R=S47J::New();Vec(R,TEXT("road_local_m"),P);Corners.Add(MakeShared<FJsonValueObject>(R));MaxX=FMath::Max(MaxX,P.X);}
    O->SetArrayField(TEXT("oriented_collision_corners"),Corners);S47J::Set(O,TEXT("collision_half_extents_m"),TArray<double>{HalfWidth,HalfLength});S47J::Set(O,TEXT("max_footprint_road_x"),MaxX);
   }
   if(const auto* T=M->World->TruckHere()){Vec(O,TEXT("actual_parked_car_three_m"),T->Position);S47J::Set(O,TEXT("actual_parked_heading_rad"),T->Rotation.Y);}
   Vec(O,TEXT("walker_three_m"),M->Player->Pos);Vec(O,TEXT("walker_road_local_m"),RoadPoint(M->Player->Pos,Leg()));S47J::Set(O,TEXT("walker_radius_m"),M->Player->Radius);S47J::Set(O,TEXT("clock_seconds"),M->Story->Clock());return O;
  }
  FJ Camera()
  {
   auto O=S47J::New();auto* P=Main()->Player.Get();Vec(O,TEXT("eye_three_m"),S47Three::ToThree(P->EyeUE()));Vec(O,TEXT("forward_three"),S47Three::ToThree(P->ForwardUE()*100.));
   auto* PC=Cast<APlayerController>(P->GetController());const auto* CameraManager=PC?PC->PlayerCameraManager.Get():nullptr;
   S47J::Set(O,TEXT("camera_manager_available"),CameraManager!=nullptr);
   if(CameraManager)
   {
    // Public final POV, after camera modifiers; Walker keeps its camera component private.
    const FMinimalViewInfo& View=CameraManager->GetCameraCacheView();
    S47J::Set(O,TEXT("camera_manager"),CameraManager->GetPathName());S47J::Set(O,TEXT("actual_world_transform"),Transform(FTransform(View.Rotation,View.Location)));
    S47J::Set(O,TEXT("actual_horizontal_fov_degrees"),double(CameraManager->GetFOVAngle()));S47J::Set(O,TEXT("constrain_aspect"),bool(View.bConstrainAspectRatio));
    S47J::Set(O,TEXT("walker_eye_to_final_pov_delta_m"),FVector::Dist(P->EyeUE(),View.Location)/100.);
   }
   FVector2D Size=FVector2D::ZeroVector;if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->GetViewportSize(Size);S47J::Set(O,TEXT("viewport"),TArray<double>{Size.X,Size.Y});return O;
  }
  FJ Components()
  {
   auto O=S47J::New();auto* W=Web.Get();TArray<TSharedPtr<FJsonValue>> Rows;int32 VisibleRanch=0;
   for(int32 I=0;I<W->PartCount();++I)
   {
    const auto* N=W->Node(I);if(!N)continue;
    const bool Family=N->Geo>=910&&N->Geo<=912,SharedRanch=N->Name.StartsWith(TEXT("highway:ranch"));
    if(!(Family||SharedRanch||I==1240||I==1289||I==952||I==976||I==959||I==1250||(I>=983&&I<=990)))continue;
    auto R=S47J::New();S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("parent"),N->Parent);S47J::Set(R,TEXT("geo"),N->Geo);S47J::Set(R,TEXT("name"),N->Name);S47J::Set(R,TEXT("authored_visible"),N->bVis);S47J::Set(R,TEXT("metadata_path_shown"),W->ObjAt(I).Shown());
    if(auto* C=N->Comp.Get())
    {
     S47J::Set(R,TEXT("component"),C->GetPathName());S47J::Set(R,TEXT("attach_parent"),GetPathNameSafe(C->GetAttachParent()));S47J::Set(R,TEXT("component_visible"),C->IsVisible());S47J::Set(R,TEXT("hidden_in_game"),C->bHiddenInGame);S47J::Set(R,TEXT("registered"),C->IsRegistered());S47J::Set(R,TEXT("world_transform"),Transform(C->GetComponentTransform()));
     if(auto* S=Cast<UStaticMeshComponent>(C))if(auto* Mesh=S->GetStaticMesh().Get())
     {
      S47J::Set(R,TEXT("mesh"),Mesh->GetPathName());const FBox B=S->Bounds.GetBox();Vec(R,TEXT("joined_bounds_min_three_m"),S47Three::ToThree(B.Min));Vec(R,TEXT("joined_bounds_max_three_m"),S47Three::ToThree(B.Max));S47J::Set(R,TEXT("joined_bounds_are_not_ranch_identity"),true);S47J::Set(R,TEXT("collision_enabled"),int32(S->GetCollisionEnabled()));
      TArray<FString> Materials;for(int32 Slot=0;Slot<S->GetNumMaterials();++Slot){auto* M=S->GetMaterial(Slot);Materials.Add(GetPathNameSafe(M));if(auto* MI=Cast<UMaterialInstance>(M))Materials.Add(TEXT("parent:")+GetPathNameSafe(MI->Parent.Get()));}S47J::Set(R,TEXT("materials_and_parents"),Materials);
      const auto* Render=Mesh->GetRenderData();const bool Readable=Mesh->bAllowCPUAccess&&Render&&Render->LODResources.Num()>0;
      S47J::Set(R,TEXT("cpu_lod0_readable"),Readable);
      if((Family||SharedRanch)&&Readable)
      {
       const auto& L=Render->LODResources[0];TArray<uint32> Ix;L.IndexBuffer.GetCopy(Ix);FBox Bounds(ForceInit);int32 Farm=0;bool InPlace=true;
       for(int32 T=0;T+2<Ix.Num();T+=3)
       {
        FVector Local[3],World[3];bool Inside=true;for(int32 K=0;K<3;++K)
        {
         if(Ix[T+K]>=L.VertexBuffers.PositionVertexBuffer.GetNumVertices()){Inside=false;break;}
         Local[K]=FVector(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Ix[T+K]));const auto P=S47Three::ToThree(Local[K]);Inside&=P.X>=389.99&&P.X<=485.01&&P.Z>=139.99&&P.Z<=235.01;
         World[K]=S47Three::ToThree(C->GetComponentTransform().TransformPosition(Local[K]));
        }
        if(!Inside)continue;++Farm;for(const auto& P:World){Bounds+=P;const auto P0=RoadPoint(P,Leg());InPlace&=P0.X>=389.99&&P0.X<=485.01&&P0.Z>=139.99&&P0.Z<=235.01;}
       }
       S47J::Set(R,TEXT("actual_ranch_subset_triangles"),Farm);S47J::Set(R,TEXT("ranch_subset_in_expected_active_scene_place"),InPlace&&Farm>0);
       if(Bounds.IsValid){Vec(R,TEXT("ranch_subset_world_min_three_m"),Bounds.Min);Vec(R,TEXT("ranch_subset_world_max_three_m"),Bounds.Max);}
       if(S->IsVisible()&&!S->bHiddenInGame&&S->IsRegistered()&&InPlace)VisibleRanch+=Farm;
      }
     }
    }
    Rows.Add(MakeShared<FJsonValueObject>(R));
   }
   O->SetArrayField(TEXT("rows"),Rows);S47J::Set(O,TEXT("visible_ranch_triangles_in_expected_place"),VisibleRanch);S47J::Set(O,TEXT("source_ranch_triangle_count"),300);
   if(Leg()==TEXT("saro"))LastSaroRanchTriangles=VisibleRanch;return O;
  }
  bool Snapshot(const FString& Name)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("at_seconds"),FPlatformTime::Seconds()-Began);S47J::Set(O,TEXT("frame"),double(GFrameCounter));S47J::Set(O,TEXT("pose"),Pose());S47J::Set(O,TEXT("camera"),Camera());S47J::Set(O,TEXT("components"),Components());
   TArray<TSharedPtr<FJsonValue>> Obstacles;auto* D=Main()->World->CabDrive();if(D&&D->Area)
   {
    for(int32 I=0;I<D->Area->Obstacles.Num();++I)
    {
     const auto& B=D->Area->Obstacles[I];auto R=S47J::New();S47J::Set(R,TEXT("index"),I);S47J::Set(R,TEXT("circle"),B.bCircle);
     if(B.bCircle){const auto P=RoadPoint(FVector(B.X,0,B.Z),Leg());S47J::Set(R,TEXT("road_local_circle_x_z_r"),TArray<double>{P.X,P.Z,B.R});}
     else{const auto A=RoadPoint(FVector(B.MinX,0,B.MinZ),Leg()),Z=RoadPoint(FVector(B.MaxX,0,B.MaxZ),Leg());S47J::Set(R,TEXT("road_local_box_x0_x1_z0_z1"),TArray<double>{A.X,Z.X,A.Z,Z.Z});}
     Obstacles.Add(MakeShared<FJsonValueObject>(R));
    }
    const auto P=ScenePoint(FVector(21.1,0,GateZ()),Leg());S47J::Set(O,TEXT("area_blocks_centre_beyond_gate"),D->Area->Blocked(P.X,P.Z));
   }
   O->SetArrayField(TEXT("complete_actual_active_controller_obstacles"),Obstacles);
   if(!JsonFile(Root/(Name+TEXT(".json")),O)){Result(false,TEXT("could not write actual scene/collider snapshot"));return false;}
   Snapshots.Add(MakeShared<FJsonValueObject>(O));return true;
  }
  void QueuePhoto(const FString& Name){PhotoName=Name;PhotoRequested=false;PhotoAt=FPlatformTime::Seconds();PhotoMeta=S47J::New();}
  bool Photo(bool Farm=true,bool Mailbox=false)
  {
   auto* M=Main();const double Now=FPlatformTime::Seconds();const FVector Target=ScenePoint(Mailbox?FVector(9.6,.65,S47RoadTerrain::RanchZ(9.6)-4.2):Farm?FVector(440,1.5,196):FVector(21,.2,GateZ()),Leg());
   if(M->World->bDriving)
   {
    auto* D=M->World->CabDrive();auto* T=M->World->CabTruck();if(!D||!T){Result(false,TEXT("photo lost actual cab"));return false;}
    const FQuat Body=S47Three::Euler(T->Rotation.X,T->Rotation.Y,T->Rotation.Z,TEXT("YXZ"));const auto V=Body.UnrotateVector(Target-D->CamPos);
    D->SetLook(FMath::Clamp(FMath::Atan2(-V.X,-V.Z),-1.85,1.85),FMath::Clamp(FMath::Atan2(V.Y,FVector2D(V.X,V.Z).Size()),-.85,.55));
   }
   else{const auto V=Target-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=FMath::Atan2(V.Y,FVector2D(V.X,V.Z).Size());}
   const FString File=Root/(PhotoName+TEXT(".png"));if(!PhotoRequested)
   {
    if(Now-PhotoAt<.5)return false;PhotoMeta=Camera();S47J::Set(PhotoMeta,TEXT("name"),PhotoName);S47J::Set(PhotoMeta,TEXT("pose"),Pose());Vec(PhotoMeta,TEXT("expected_landmark_three_m"),Target);
    FVector2D Pixel=FVector2D::ZeroVector;auto* PC=Cast<APlayerController>(M->Player->GetController());const bool Projects=PC&&PC->ProjectWorldLocationToScreen(S47Three::ToUE(Target),Pixel,true);
    S47J::Set(PhotoMeta,TEXT("landmark_projects"),Projects);S47J::Set(PhotoMeta,TEXT("landmark_pixel"),TArray<double>{Pixel.X,Pixel.Y});S47J::Set(PhotoMeta,TEXT("target_visibility_is_not_assumed"),true);
    FScreenshotRequest::RequestScreenshot(File,true,false);PhotoRequested=true;return false;
   }
   FJ Receipt;if(!PngReceipt(File,Receipt)){if(Now-PhotoAt>12)Result(false,TEXT("complete viewport PNG missing: ")+PhotoName);return false;}
   S47J::Set(PhotoMeta,TEXT("png"),Receipt);if(!JsonFile(Root/(PhotoName+TEXT(".capture.json")),PhotoMeta)){Result(false,TEXT("capture metadata write failed"));return false;}
   Photos.Add(MakeShared<FJsonValueObject>(PhotoMeta));PhotoName.Empty();PhotoMeta.Reset();return true;
  }
  void PhysicalFailure(const FString& Why)
  {
   if(!PendingFailure.IsEmpty())return;PendingFailure=Why;Brake();Main()->Player->TestMove.Reset();Next(90,TEXT("actual physical failure; retain first offending pose, brake normally, then capture viewport"));
   if(!Snapshot(TEXT("first-physical-failure")))return;QueuePhoto(TEXT("failure-at-gate"));
  }
  bool Monitor(double Now)
  {
   auto* M=Main();if(!Tracking||!M->World->bDriving)return true;auto* D=M->World->CabDrive();if(!D){Result(false,TEXT("physical route lost cab controller"));return false;}
   const FVector P=CarRoad();const double Dt=FMath::Max(.001,Now-Last),Moved=FVector2D(P.X-LastRoad.X,P.Z-LastRoad.Z).Size();
   if(Moved>FMath::Max(1.5,FMath::Max(FMath::Abs(LastSpeed),FMath::Abs(D->Speed))*Dt+1.)){Result(false,TEXT("canonical vehicle discontinuity; no placement recovery"));return false;}
   DriveMetres+=Moved;LastRoad=P;Last=Now;LastSpeed=D->Speed;
   if(LastLeg!=Leg())
   {
    if(!(LastLeg==TEXT("road")&&Leg()==TEXT("saro"))){Result(false,TEXT("unexpected ordinary leg transfer"));return false;}
    ++Transitions;LastLeg=Leg();if(!Snapshot(TEXT("actual-road-to-saro-transfer")))return false;
   }
   if(Now-LastTrace>=.1){auto R=Pose();S47J::Set(R,TEXT("at_seconds"),Now-Began);Trace.Add(MakeShared<FJsonValueObject>(R));LastTrace=Now;}
   if(AcrossGate(Footprint(P,D->Heading))){PhysicalFailure(TEXT("measured oriented vehicle collision footprint crossed the visible closed pipe-gate plane"));return false;}
   if(FVector2D(P.X-ProgressRoad.X,P.Z-ProgressRoad.Z).Size()>.25){ProgressRoad=P;ProgressAt=Now;}
   if(Stage!=8&&Stage!=90&&Now-ProgressAt>18&&M->World->TestInput){Result(false,TEXT("physical route stalled before the gate; no pose/heading fallback"));return false;}return true;
  }
  FS47DriveIn Input()
  {
   FS47DriveIn I;auto* M=Main();auto* D=M&&M->World?M->World->CabDrive():nullptr;if(!D||Path.IsEmpty())return I;const auto P=CarRoad();
   while(Cursor<Path.Num()-1&&FVector2D(Path[Cursor].X-P.X,Path[Cursor].Z-P.Z).Size()<.85)++Cursor;
   const double Final=FVector2D(Path.Last().X-P.X,Path.Last().Z-P.Z).Size();
   if(Final<.3){I.Throttle=D->Speed>.035?-1:D->Speed<-.035?1:0;PathDone=FMath::Abs(D->Speed)<.07;return I;}
   FVector Aim=Path[Cursor];double Look=2.5;FVector From=P;
   for(int32 K=Cursor;K<Path.Num();++K){const auto Delta=Path[K]-From;const double Len=FVector2D(Delta.X,Delta.Z).Size();if(Len>=Look){Aim=From+Delta*(Look/Len);break;}Look-=Len;Aim=Path[K];From=Path[K];}
   const auto Delta=Aim-P;const double Right=Delta.X*FMath::Cos(D->Heading)-Delta.Z*FMath::Sin(D->Heading),Dist2=FMath::Max(.1,Delta.X*Delta.X+Delta.Z*Delta.Z);
   I.Steer=D->SteerFor(2*Right/Dist2);const double Wanted=FMath::Min(MaxSpeed,FMath::Max(.18,Final*.8))*(Reverse?-1:1);
   I.Throttle=FMath::Clamp((Wanted-D->Speed)*.8,-1.,1.);return I;
  }
  void Follow(const TArray<FVector>& Points,double Speed,bool Back=false)
  {
   Path=Points;Cursor=0;PathDone=false;MaxSpeed=Speed;Reverse=Back;ProgressAt=FPlatformTime::Seconds();ProgressRoad=CarRoad();
   auto* M=Main();M->World->Autopilot.Reset();TWeakPtr<FControl> Weak=AsShared();M->World->TestInput=[Weak]{const auto S=Weak.Pin();return S?S->Input():FS47DriveIn();};
   if(auto* D=M->World->CabDrive())D->SetLook(0,-.04);
  }
  void WalkTo(const TArray<FVector>& Points)
  {Walk=Points;WalkCursor=0;LastFoot=Main()->Player->Pos;WalkLast=FPlatformTime::Seconds();}
  bool WalkStep(double Now)
  {
   auto* P=Main()->Player.Get();const double Moved=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();
   if(Moved>FMath::Max(.5,(Now-WalkLast)*6+.1)){Result(false,TEXT("Walker discontinuity; no pose fallback"));return false;}
   WalkMetres+=Moved;LastFoot=P->Pos;WalkLast=Now;const auto R=RoadPoint(P->Pos,Leg());
   if(R.X+P->Radius>GateX+.015&&FMath::Abs(R.Z-GateZ())<3.2){PhysicalFailure(TEXT("actual Walker footprint crossed the visible closed pipe gate"));return false;}
   if(WalkCursor>=Walk.Num()){P->TestMove.Reset();return true;}
   const auto Delta=Walk[WalkCursor]-P->Pos;const double Dist=FVector2D(Delta.X,Delta.Z).Size();
   if(Dist<.09){P->TestMove.Reset();++WalkCursor;return false;}
   if(Now-At>45){Result(false,TEXT("physical mailbox/gate walk blocked before target"));return false;}
   P->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);P->Pitch=-.05;P->TestMove=FVector2D(0,-FMath::Min(1.,Dist/.4));P->bTestRun=false;return false;
  }
  bool SameDoors()const
  {const auto Now=Web->DoorStates();if(Now.Num()!=SavedDoors.Num())return false;for(const auto& V:SavedDoors){const bool* N=Now.Find(V.Key);if(!N||*N!=V.Value)return false;}return true;}

  void Tick()
  {
   const TSharedRef<FControl> KeepAlive=AsShared();if(Done)return;const double Now=FPlatformTime::Seconds();auto* M=Main();
   if(Now-Began>360){Result(false,TEXT("360 second bounded FarmAccess timeout"));return;}
   if(!M||!M->Flow||!M->World||!M->Player||!M->Story||!M->UI||!M->Inter||!M->Evening||S47Startup::Blocking()){Ready=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){Ready=0;return;}if(Ready==0)Ready=Now;if(Now-Ready<2)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Evening->Commute()&&M->World->bDriving&&M->World->Leg==TEXT("road"),TEXT("ordinary fresh commute exists before sole declared start fixture")))return;
    PackBefore=S47J::Obj(M->Flow->GameState(),TEXT("evening"));PackBefore=S47J::Obj(PackBefore,TEXT("pack"));
    auto Car=S47J::New();S47J::Set(Car,TEXT("leg"),TEXT("road"));S47J::Set(Car,TEXT("x"),8001.8);S47J::Set(Car,TEXT("y"),0.);S47J::Set(Car,TEXT("z"),535.);S47J::Set(Car,TEXT("h"),0.);
    if(!JsonFile(Root/TEXT("sole-explicit-start-fixture.json"),Car)){Result(false,TEXT("fixture provenance write failed"));return;}
    Started=true;M->World->BeginCommute(Car);Next(1,TEXT("sole explicit BeginCommute saved-car fixture: road X1.8 Z535 north; all later movement is ordinary control"));return;
   }
   if(!PendingFailure.IsEmpty())
   {Brake();if(Now-At<.5)return;if(FMath::Abs(M->World->DrivingSpeed())>.08&&Now-At<3)return;if(Photo(false))Result(false,PendingFailure);return;}
   if(!Monitor(Now)||Done)return;if(M->Flow->bStarting||M->World->bBusy)return;
   if(Stage!=17&&M->Mode!=US47Main::EMode::Play){Result(false,TEXT("unexpected mode during ordinary physical route"));return;}
   if(Stage==1)
   {
    if(!M->World->bDriving)return;auto* D=M->World->CabDrive();
    if(!Check(D&&Leg()==TEXT("road")&&D->Pos.Equals(FVector(8001.8,D->Pos.Y,535),.015)&&FMath::Abs(D->Heading)<.001,TEXT("declared start fixture settled; no subsequent vehicle pose writes")))return;
    Tracking=true;LastLeg=Leg();LastRoad=ProgressRoad=CarRoad();Last=ProgressAt=Now;Brake();if(!Snapshot(TEXT("road-start-535")))return;QueuePhoto(TEXT("road-start-ranch"));Next(2,TEXT("actual road cockpit and ranch subset before transition"));return;
   }
   if(Stage==2){if(!Photo())return;Follow({FVector(1.8,0,486)},6);Next(3,TEXT("physical northbound control to before handoff"));return;}
   if(Stage==3){if(!PathDone)return;Brake();if(!Snapshot(TEXT("road-before-handoff")))return;QueuePhoto(TEXT("road-before-handoff-ranch"));Next(4,TEXT("stopped normally before boundary; capture unchanged ranch landmark"));return;}
   if(Stage==4){if(!Photo())return;Follow({FVector(1.8,0,462)},5);Next(5,TEXT("physical road to SARO transfer; no coordinate fixture"));return;}
   if(Stage==5)
   {
    if(!PathDone)return;if(!Check(Transitions==1&&Leg()==TEXT("saro"),TEXT("ordinary road to SARO handoff measured continuously")))return;
    Brake();if(!Snapshot(TEXT("saro-after-handoff")))return;QueuePhoto(TEXT("saro-after-handoff-ranch"));Next(6,TEXT("same ranch landmark from actual SARO cockpit; invisibility is recorded, not bypassed"));return;
   }
   if(Stage==6)
   {
    if(!Photo())return;TArray<FVector> Points;for(double Z=450;Z>214;Z-=12)Points.Add(FVector(1.8,0,Z));
    const double Z0=S47RoadTerrain::RanchZ(16.5),Cz=Z0+6;Points.Append({FVector(1.8,0,214),FVector(1.8,0,208),FVector(1.8,0,Cz)});
    for(int32 I=1;I<=18;++I){const double A=UE_DOUBLE_PI+I*UE_DOUBLE_PI/36;Points.Add(FVector(7.8+6*FMath::Cos(A),0,Cz+6*FMath::Sin(A)));}
    Points.Append({FVector(10,0,Z0),FVector(13,0,Z0),FVector(16.5,0,Z0)});Follow(Points,3);Next(7,TEXT("ordinary highway driving and right turn along existing ranch apron"));return;
   }
   if(Stage==7)
   {
    if(!PathDone)return;Brake();if(!Snapshot(TEXT("saro-apron-before-port")))return;QueuePhoto(TEXT("saro-apron-port"));Next(71,TEXT("actual arrival before controlled low-speed gate approach"));return;
   }
   if(Stage==71){if(!Photo(false))return;Follow({FVector(20.2,0,S47RoadTerrain::RanchZ(16.5))},1.1);Next(8,TEXT("physical low-speed pressure towards closed gate; fail only on actual footprint crossing"));return;}
   if(Stage==8)
   {
    auto* D=M->World->CabDrive();const auto P=CarRoad();double Front=-1e30;for(const auto& C:Footprint(P,D->Heading))Front=FMath::Max(Front,C.X);
    if(Front>20.3&&FMath::Abs(D->Speed)<.06){if(StalledAt==0)StalledAt=Now;}else StalledAt=0;
    if(StalledAt>0&&Now-StalledAt>1.2&&Now-At>2)
    {
     CarBlocked=true;ParkedRoad=P;ParkedHeading=D->Heading;Brake();if(!Snapshot(TEXT("car-stopped-by-closed-port")))return;QueuePhoto(TEXT("car-port-blocked"));Next(9,TEXT("measured oriented footprint stopped on near side while throttle requested forward motion"));return;
    }
    if(Now-At>12)PhysicalFailure(TEXT("gate approach stalled away from visible gate or failed to demonstrate a physical stop"));return;
   }
   if(Stage==9)
   {
    if(!Photo(false))return;if(!Check(CarBlocked&&M->World->CanGetOut(),TEXT("normal safe egress available after actual blocked approach")))return;
    Tracking=false;M->World->TestInput=nullptr;if(!Check(M->World->GetOutHere(),TEXT("ordinary GetOutHere, no player placement")))return;Next(10,TEXT("wait for normal parked-car/Walker handover"));return;
   }
   if(Stage==10)
   {
    if(Now-At<.8)return;EgressRoad=RoadPoint(M->Player->Pos,Leg());
    WalkTo({ScenePoint(FVector(14.2,0,EgressRoad.Z),Leg()),ScenePoint(FVector(11,0,194.3),Leg()),ScenePoint(FVector(9.6,0,193.2),Leg())});Next(11,TEXT("actual Walker around rear of car to existing ALDERETE mailbox"));return;
   }
   if(Stage==11)
   {
    if(!WalkStep(Now))return;if(!Snapshot(TEXT("walker-mailbox")))return;QueuePhoto(TEXT("walker-mailbox-port"));Next(111,TEXT("actual mailbox area, no new quest or interaction fixture"));return;
   }
   if(Stage==111)
   {
    if(!Photo(false,true))return;WalkTo({ScenePoint(FVector(11,0,194.3),Leg()),ScenePoint(FVector(14.2,0,194.3),Leg()),ScenePoint(FVector(19.5,0,194.3),Leg())});Next(12,TEXT("walk to clear northern side of closed gate, away from parked car"));return;
   }
   if(Stage==12)
   {
    if(!WalkStep(Now))return;Next(13,TEXT("actual forward Walker input against closed port"));LastFoot=M->Player->Pos;WalkLast=Now;return;
   }
   if(Stage==13)
   {
    auto* P=M->Player.Get();const double Moved=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();WalkMetres+=Moved;LastFoot=P->Pos;WalkLast=Now;const auto R=RoadPoint(P->Pos,Leg());
    if(R.X+P->Radius>GateX+.015){PhysicalFailure(TEXT("actual Walker footprint passed through closed port"));return;}
    P->Yaw=-UE_DOUBLE_PI/2;P->Pitch=-.1;P->TestMove=FVector2D(0,-1);P->bTestRun=false;
    if(Moved<.003&&R.X>19.8){if(StalledAt==0)StalledAt=Now;}else StalledAt=0;
    if(StalledAt>0&&Now-StalledAt>.8&&Now-At>1.5)
    {
     P->TestMove.Reset();PersonBlocked=true;if(!Snapshot(TEXT("walker-blocked-by-port")))return;
     WalkTo({ScenePoint(FVector(19.5,0,194.3),Leg()),ScenePoint(FVector(14.2,0,194.3),Leg()),ScenePoint(FVector(14.2,0,EgressRoad.Z),Leg()),ScenePoint(EgressRoad,Leg())});Next(14,TEXT("actual Walker returns around car to original ordinary egress spot"));return;
    }
    if(Now-At>8)PhysicalFailure(TEXT("Walker probe did not demonstrate the closed gate"));return;
   }
   if(Stage==14)
   {
    if(!WalkStep(Now))return;const auto* It=M->Inter->Get(TEXT("truck"));if(!Check(It&&It->Object.IsValid(),TEXT("original car interaction proxy remains available")))return;
    const auto V=It->Object.WorldPosition()-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=FMath::Atan2(V.Y,FVector2D(V.X,V.Z).Size());Next(141,TEXT("settle ordinary car-use view"));return;
   }
   if(Stage==141)
   {
    if(Now-At<.25)return;const auto Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());if(!Check(Pick==TEXT("truck"),TEXT("actual interaction ray picks original parked-car proxy")))return;
    if(!M->Evening->Use(Pick))M->Inter->UseId(Pick);Next(142,TEXT("normal Evening/HUD interaction path enters the same car"));return;
   }
   if(Stage==142)
   {
    if(!M->World->bDriving)return;const auto P=CarRoad();if(!Check(FVector2D(P.X-ParkedRoad.X,P.Z-ParkedRoad.Z).Size()<.1&&FMath::Abs(Angle(M->World->CabDrive()->Heading-ParkedHeading))<.02,TEXT("ordinary re-entry retained actual parked position/heading")))return;
    Tracking=true;LastRoad=ProgressRoad=P;Last=ProgressAt=Now;LastSpeed=M->World->DrivingSpeed();LastLeg=Leg();const double Z0=S47RoadTerrain::RanchZ(16.5),Cz=Z0+6;TArray<FVector> Points={FVector(16,0,Z0),FVector(13,0,Z0),FVector(10,0,Z0),FVector(7.8,0,Z0)};
    for(int32 I=17;I>=0;--I){const double A=UE_DOUBLE_PI+I*UE_DOUBLE_PI/36;Points.Add(FVector(7.8+6*FMath::Cos(A),0,Cz+6*FMath::Sin(A)));}
    Points.Append({FVector(1.8,0,206),FVector(1.8,0,210)});Follow(Points,1.5,true);Next(15,TEXT("ordinary reverse control backs along same apron and turn to highway; no pose writes"));return;
   }
   if(Stage==15)
   {
    if(!PathDone)return;Brake();if(!Snapshot(TEXT("returned-to-highway")))return;QueuePhoto(TEXT("returned-highway-ranch"));Next(151,TEXT("same ranch and actual highway cockpit after reverse"));return;
   }
   if(Stage==151)
   {
    if(!Photo())return;Tracking=false;M->World->TestInput=nullptr;if(!Check(M->World->CanGetOut()&&M->World->GetOutHere(),TEXT("ordinary final highway egress for save boundary")))return;Next(16,TEXT("normal parking before isolated disk/title/Continue"));return;
   }
   if(Stage==16)
   {
    if(Now-At<.8)return;if(!Check(CarBlocked&&PersonBlocked&&M->Flow->GameSaveBlock().IsEmpty(),TEXT("both physical barriers demonstrated and legal gameplay save boundary")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();SavedDoors=Web->DoorStates();
    if(!Check(SameJson(PackBefore,S47J::Obj(S47J::Obj(BeforeSave,TEXT("evening")),TEXT("pack"))),TEXT("commute story/quest pack unchanged by entire farm detour")))return;
    // Same isolated case0-to-case1 disk fixture as HomesContract; do not bypass Flow's dev-save guard.
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("ENV04 farm access control");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;FString Error;
    const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT DISK FIXTURE: unchanged actual case0 snapshot copied to isolated case1: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();const auto Disk=Reader.Load(Meta->Id);if(!Check(SameJson(Disk,BeforeSave),TEXT("fresh disk reader matches whole actual snapshot")))return;
    if(!JsonFile(Root/TEXT("before-continue.disk.json"),Disk)){Result(false,TEXT("disk evidence write failed"));return;}SaveId=Meta->Id;M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(17,TEXT("ordinary title/Continue with isolated disk; no restore helper"));return;
   }
   if(Stage==17)
   {
    if(Now-At<.6)return;const auto Save=M->Flow->Saves.ContinueSave();if(!Check(M->Mode==US47Main::EMode::Title&&Save&&Save->Id==SaveId,TEXT("ordinary title resolves the isolated actual save")))return;
    M->Flow->OpenTestView(TEXT("continue"));Next(18,TEXT("ordinary Continue/OpenSave/Flow.StartGame"));return;
   }
   if(Stage==18)
   {
    if(Now-At<.8)return;if(!M->Flow->SaveBlock().IsEmpty()){if(Now-At>15)Result(false,TEXT("post-Continue ordinary save guard did not clear: ")+M->Flow->SaveBlock());return;}
    const auto Actual=M->Flow->GameState();const auto E0=S47J::Obj(BeforeSave,TEXT("evening")),E1=S47J::Obj(Actual,TEXT("evening")),A=S47J::Obj(E0,TEXT("car")),B=S47J::Obj(E1,TEXT("car"));
    bool Okay=M->Evening->Commute()&&!M->World->bDriving&&M->World->Area==TEXT("saro")&&M->Player->Pos.Equals(SavedFoot,.015)&&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Story->Clock()-SavedClock)<.001&&SameDoors()&&SameJson(PackBefore,S47J::Obj(E1,TEXT("pack")));
    for(const TCHAR* K:{TEXT("x"),TEXT("y"),TEXT("z"),TEXT("h")})Okay&=A&&B&&FMath::Abs(S47J::Num(A,K)-S47J::Num(B,K))<.015;
    Okay&=A&&B&&!S47J::Bool(B,TEXT("driving"))&&S47J::Str(A,TEXT("at"))==S47J::Str(B,TEXT("at"))&&S47J::Str(A,TEXT("leg"))==S47J::Str(B,TEXT("leg"));
    auto Report=S47J::New();S47J::Set(Report,TEXT("before"),BeforeSave);S47J::Set(Report,TEXT("actual"),Actual);S47J::Set(Report,TEXT("pass"),Okay);if(!JsonFile(Root/TEXT("continue-readback.json"),Report)){Result(false,TEXT("Continue readback write failed"));return;}
    if(!Check(Okay,TEXT("full ordinary Flow Continue retained car/player/clock/doors/quest pack")))return;
    FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary Flow.WriteSave after Continue: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();if(!Check(SameJson(Reader.Load(Meta->Id),Actual),TEXT("fresh disk reads complete normal post-Continue save")))return;
    if(!Snapshot(TEXT("after-full-flow-continue")))return;
    if(!Check(LastSaroRanchTriangles==300,TEXT("same 300 ranch triangles remain visible at expected location in SARO; joined bounds do not count as visibility")))return;
    for(const auto& P:Photos){FJ Fresh;const auto Receipt=S47J::Obj(P->AsObject(),TEXT("png"));if(!Check(PngReceipt(S47J::Str(Receipt,TEXT("file")),Fresh)&&S47J::Str(Fresh,TEXT("sha1"))==S47J::Str(Receipt,TEXT("sha1")),TEXT("final actual viewport PNG CRC/IEND/hash receipt")))return;}
    Result(true,TEXT("continuous handoff/arrival, actual car and Walker barriers, original car re-entry/reverse, visible ranch and unchanged quest pack through full disk/title/Continue"));
   }
  }
 };
}

void AS47Web::StartFarmAccessContract()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47FarmAccessContract")))return;
 FString UserDir,Evening,Value;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 const bool Safe=(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir);
 bool Conflict=false;for(const TCHAR* K:{TEXT("S47Case="),TEXT("S47EveningRoute="),TEXT("S47Drive="),TEXT("S47Walk="),TEXT("S47Face="),TEXT("S47WalkFrom="),TEXT("S47Auto="),TEXT("S47Hide="),TEXT("S47Jump="),TEXT("S47HorrorContract="),TEXT("S47ProductionSuite="),TEXT("S47MotelTv="),TEXT("S47Town="),TEXT("S47Roadside=")})Conflict|=FParse::Value(FCommandLine::Get(),K,Value);
 for(const TCHAR* K:{TEXT("S47Title"),TEXT("S47SmokeTest"),TEXT("S47CalicheHomesContract"),TEXT("S47CalicheHomesContractFallback"),TEXT("S47MotelV2Route"),TEXT("S47T01DriveContract"),TEXT("S47NightSequenceContract"),TEXT("S47EveningEntrance"),TEXT("S47GetOut"),TEXT("S47PropsOff"),TEXT("S47LegacyHighway"),TEXT("S47LegacyCalicheStreet"),TEXT("S47SaroParking"),TEXT("S47ProductionViews"),TEXT("S47HorrorArrival")})Conflict|=FParse::Param(FCommandLine::Get(),K);
 if(!Safe||Conflict||!FParse::Value(FCommandLine::Get(),TEXT("S47Evening="),Evening)||Evening!=TEXT("commute"))
 {UE_LOG(LogS47FarmAccess,Error,TEXT("S47: FarmAccess contract FAIL requires isolated /tmp UserDir, -S47Evening=commute, ordinary assets, case0 and no competing route/pose driver"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto S=MakeShared<FControl>();S->Web=this;S->Root=FPaths::ProjectSavedDir()/TEXT("FarmAccessContract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*S->Root,true)){UE_LOG(LogS47FarmAccess,Error,TEXT("S47: FarmAccess contract FAIL evidence directory"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 GetWorldTimerManager().SetTimer(S->Timer,[S]{S->Tick();},.05f,true);
 UE_LOG(LogS47FarmAccess,Display,TEXT("S47: FarmAccess contract armed; one declared road-Z535 commute fixture then continuous physical handoff/port approach; evidence=%s"),*S->Root);
}
