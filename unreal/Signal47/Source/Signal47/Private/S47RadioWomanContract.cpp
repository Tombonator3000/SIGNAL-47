// HUM01: declared parked-car/clock fixtures, ordinary egress/Walker/interaction and full disk Continue.
#include "S47RadioWomanContract.h"
#include "S47Web.h"
#include "S47Caliche.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInterface.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogS47RadioWomanContract,Log,All);
namespace S47RadioWomanContract
{
 using FJ=S47J::FObj;
 bool Write(const FString& File,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*File);}
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 double Angle(double V){return FMath::Atan2(FMath::Sin(V),FMath::Cos(V));}
 void Vec(const FJ& O,const TCHAR* Key,const FVector& P){S47J::Set(O,Key,TArray<double>{P.X,P.Y,P.Z});}
 bool Png(const FString& File,FJ& Out)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*File))return false;
  const uint8 Magic[]={137,80,78,71,13,10,26,10};if(B.Num()<33||FMemory::Memcmp(B.GetData(),Magic,8))return false;
  auto U32=[&](int64 I){return (uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;uint32 W=0,H=0;int32 Chunks=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 N=U32(At);if(int64(N)>B.Num()-At-12)return false;const uint8* T=B.GetData()+At+4;
   if(FCrc::MemCrc32(T,N+4)!=U32(At+8+N))return false;
   if(Chunks==0){if(N!=13||FMemory::Memcmp(T,"IHDR",4))return false;W=U32(At+8);H=U32(At+12);}
   if(!FMemory::Memcmp(T,"IDAT",4))Idat+=N;++Chunks;At+=int64(N)+12;
   if(!FMemory::Memcmp(T,"IEND",4)){End=N==0;break;}
  }
  if(!End||At!=B.Num()||Idat<=0||W<320||H<200)return false;
  FSHAHash Hash;FSHA1::HashBuffer(B.GetData(),B.Num(),Hash.Hash);Out=S47J::New();
  S47J::Set(Out,TEXT("file"),File);S47J::Set(Out,TEXT("sha1"),Hash.ToString());S47J::Set(Out,TEXT("bytes"),B.Num());
  S47J::Set(Out,TEXT("width"),double(W));S47J::Set(Out,TEXT("height"),double(H));
  S47J::Set(Out,TEXT("all_chunk_crc_verified"),true);S47J::Set(Out,TEXT("iend_at_eof"),true);return true;
 }
 struct FBenchDiagnostic:TSharedFromThis<FBenchDiagnostic>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root;bool Done=false,PostRepair=false;
  double Began=FPlatformTime::Seconds(),Ready=0;
  bool Settled()const
  {
   auto* W=Web.Get();auto* M=W&&W->GetWorld()?US47Main::Get(W):nullptr;
   return W&&M&&M->Flow&&M->World&&S47Startup::Ready()&&!S47Startup::Blocking()&&
    M->Mode==US47Main::EMode::Play&&!M->Flow->bStarting&&!M->Flow->bRestoringNow&&!M->World->bBusy&&W->Area()==TEXT("diner");
  }
  void Result(bool TimedOut,const FString& Why)
  {
   const auto KeepAlive=AsShared();if(Done)return;Done=true;const double Now=FPlatformTime::Seconds();auto* W=Web.Get();auto* M=W&&W->GetWorld()?US47Main::Get(W):nullptr;
   const FJ Actual=W?W->RadioWomanReadback():S47J::New();const FJ* Diagnostic=nullptr;bool Active=true,Intact=false,Requested=false,Valid=false;
   const bool NativeValid=Actual->TryGetBoolField(TEXT("active"),Active)&&!Active&&
    Actual->TryGetBoolField(TEXT("fallback_original_bindings_intact"),Intact)&&Intact&&
    Actual->TryGetBoolField(TEXT("bench_diagnostic_requested"),Requested)&&Requested&&
    Actual->TryGetObjectField(TEXT("bench_diagnostics"),Diagnostic)&&(*Diagnostic)->TryGetBoolField(TEXT("measurement_valid"),Valid)&&Valid;
   const bool PhaseValid=!PostRepair||(Diagnostic&&Diagnostic->IsValid()&&S47J::Bool(Actual,TEXT("bench_postrepair_diagnostic_requested"))
     &&S47J::Str(*Diagnostic,TEXT("source_geometry_phase"))==TEXT("POST_REPAIR_RIGID_SOURCE_FRAME_NATIVE_QUERY"));
    const bool Measured=!TimedOut&&Settled()&&Ready>0&&Now-Ready>=5.&&NativeValid&&PhaseValid;
   auto O=S47J::New();S47J::Set(O,TEXT("schema_version"),1);S47J::Set(O,TEXT("status"),Measured?TEXT("MEASURED"):TEXT("REJECTED"));
   S47J::Set(O,TEXT("reason"),Measured?TEXT("fresh diner measurements, original figure/radio intact; no support or runtime acceptance"):Why);
   S47J::Set(O,TEXT("expected_postrepair_phase"),PostRepair);S47J::Set(O,TEXT("phase_matches_request"),PhaseValid);
    S47J::Set(O,TEXT("contact_accepted"),false);S47J::Set(O,TEXT("runtime_accepted"),false);
    S47J::Set(O,TEXT("native_readback"),Actual);S47J::Set(O,TEXT("timeout"),TimedOut);S47J::Set(O,TEXT("elapsed_wall_seconds"),Now-Began);S47J::Set(O,TEXT("settled_wall_seconds"),Ready>0?Now-Ready:0.);
   S47J::Set(O,TEXT("minimum_settled_seconds"),5.);S47J::Set(O,TEXT("maximum_wait_seconds"),20.);S47J::Set(O,TEXT("area_at_capture"),W?W->Area():TEXT("missing web"));
   S47J::Set(O,TEXT("startup_ready"),S47Startup::Ready());S47J::Set(O,TEXT("startup_blocking"),S47Startup::Blocking());S47J::Set(O,TEXT("play"),M&&M->Mode==US47Main::EMode::Play);
   S47J::Set(O,TEXT("flow_starting"),!M||!M->Flow||M->Flow->bStarting);S47J::Set(O,TEXT("flow_restoring"),M&&M->Flow&&M->Flow->bRestoringNow);S47J::Set(O,TEXT("world_busy"),M&&M->World&&M->World->bBusy);
   S47J::Set(O,TEXT("scope"),TEXT("Single read-only diagnostic after ordinary startup/Flow Play and existing WalkFrom diner. No player/world/clock setter, screenshot, exit or other contract. Existing smoke/shot owns capture and shutdown."));
   const bool Written=Write(Root/TEXT("result.json"),O);if(W&&W->GetWorld())W->GetWorldTimerManager().ClearTimer(Timer);
   if(Measured&&Written){UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioBenchDiagnostic MEASURED evidence=%s"),*Root);}
   else{UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioBenchDiagnostic REJECTED evidence=%s reason=%s written=%d"),*Root,*Why,Written);}
  }
  void Tick()
  {
   const auto KeepAlive=AsShared();if(Done)return;const double Now=FPlatformTime::Seconds();
   if(!Settled())Ready=0;else if(Ready==0)Ready=Now;
   if(Now-Began>=20.){Result(true,TEXT("diner/startup/Flow Play did not remain ready for5 real seconds within20 seconds"));return;}
   if(Ready>0&&Now-Ready>=5.)Result(false,TEXT("fresh Readback failed inactive/original-bindings/native-bench-diagnostic checks"));
  }
 };
 struct FControl:TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root,PhotoName,SaveId;
  bool Done=false,Started=false,Requested=false,Fallback=false;
  int32 Stage=0,Checks=0,Waypoint=0,Schedule=0,NativeNode=INDEX_NONE;
  double Began=FPlatformTime::Seconds(),At=Began,Ready=0,PhotoAt=0,Last=Began,WalkMetres=0,ProgressAt=Began;
  FVector LastFoot=FVector::ZeroVector,ProgressFoot=FVector::ZeroVector,SavedFoot=FVector::ZeroVector;
  double SavedYaw=0,SavedPitch=0,SavedClock=0;TMap<FString,bool> Doors;TArray<FVector> Path;
  FJ BeforeSave,PackBefore;TArray<FString> SeenBefore;
  TArray<TSharedPtr<FJsonValue>> Photos,Snapshots,Events;
  US47Main* Main()const{return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  UStaticMeshComponent* Body()const{const auto* N=Web->Node(NativeNode);return N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;}
  void Result(bool Pass,const FString& Why)
  {
   const auto Keep=AsShared();if(Done)return;Done=true;auto* M=Main();
   if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(Body())Body()->SetForcedLodModel(0);
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("walk_metres"),WalkMetres);S47J::Set(O,TEXT("explicit_off_fallback"),Fallback);
   if(Web.IsValid())S47J::Set(O,TEXT("native_readback"),Web->RadioWomanReadback());
   O->SetArrayField(TEXT("snapshots"),Snapshots);O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("events"),Events);
   S47J::Set(O,TEXT("scope"),TEXT("Explicit saved-car start road x=-6 z=2138, then ordinary GetOutHere/Walker/Inter.Pick and UseId. Caliche.Choice is API-driven. Declared clock fixtures exercise actual TownStep schedule. Actual case0 snapshot copied unchanged to isolated case1 disk for ordinary title/Continue. Human input, listening, FPS, whole night/full matrix and installation unverified."));
   const bool Written=Write(Root/TEXT("result.json"),O);if(Web.IsValid())Web->GetWorldTimerManager().ClearTimer(Timer);
   if(!Written){UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL stage=%d: final result could not be written: %s"),Stage,*Root);FPlatformMisc::RequestExitWithStatus(false,1);return;}
   if(Pass){UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioWoman contract PASS checks=%d walk=%.3fm evidence=%s"),Checks,WalkMetres,*Root);FPlatformMisc::RequestExit(false);}
   else{UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL stage=%d: %s evidence=%s"),Stage,*Why,*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Good,const FString& Why){++Checks;if(!Good){Result(false,Why);return false;}return true;}
  void Next(int32 S,const FString& Why){Stage=S;At=FPlatformTime::Seconds();auto E=S47J::New();S47J::Set(E,TEXT("stage"),S);S47J::Set(E,TEXT("reason"),Why);Events.Add(MakeShared<FJsonValueObject>(E));UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47 RadioWoman stage %d %s"),S,*Why);}
  FVector Target()const{return Web->Obj(TEXT("world.town.proxies.woman")).WorldPosition();}
  void Look(double Raise=0){auto* M=Main();const auto D=Target()+FVector(0,Raise,0)-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
  bool ShoeContact(const FJ& Snapshot)
  {
   const auto KeepAlive=AsShared();using FRows=TArray<TSharedPtr<FJsonValue>>;
   auto* W=Web.Get();auto* SceneWorld=W?W->GetWorld():nullptr;auto* C=W?Body():nullptr;const auto* Mesh=C?C->GetStaticMesh().Get():nullptr;
   const FJ EvidenceSnapshot=Snapshot.IsValid()?Snapshot:S47J::New(),Proof=S47J::New();FRows Feet;
   S47J::Set(Proof,TEXT("schema_version"),1);S47J::Set(Proof,TEXT("measurement_source"),TEXT("Independent current body LOD0 and fresh world/direct physics; no runtime-gate JSON used"));
   S47J::Set(Proof,TEXT("maximum_single_world_queries_per_point"),32);S47J::Set(Proof,TEXT("initial_ignored_actors"),0);
   S47J::Set(Proof,TEXT("entire_laundry_shell_ignored"),false);S47J::Set(Proof,TEXT("complete"),false);
   S47J::Set(Proof,TEXT("body_component"),GetPathNameSafe(C));S47J::Set(Proof,TEXT("body_mesh"),GetPathNameSafe(Mesh));
   S47J::Set(Proof,TEXT("web_area"),W?W->Area():FString());
   EvidenceSnapshot->SetObjectField(TEXT("independent_shoe_contact_query"),Proof);EvidenceSnapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);
   const auto ContactCheck=[&](bool Good,const FString& Why)
   {
    if(!Good)
    {
     S47J::Set(Proof,TEXT("failure"),Why);S47J::Set(Proof,TEXT("complete"),false);EvidenceSnapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);
     const FString FailureFile=Root/TEXT("first-shoe-contact-failure.json");S47J::Set(EvidenceSnapshot,TEXT("shoe_contact_failure_sidecar"),FailureFile);
     const bool Written=Write(FailureFile,EvidenceSnapshot);S47J::Set(EvidenceSnapshot,TEXT("shoe_contact_failure_sidecar_written"),Written);
     Snapshots.Add(MakeShared<FJsonValueObject>(EvidenceSnapshot)); // publish before Check calls Result/RequestExit
    }
    return Check(Good,Why);
   };
   if(!ContactCheck(SceneWorld&&Mesh&&C->GetComponentTransform().IsValid()&&Mesh->bAllowCPUAccess&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==3,
    TEXT("actual static shoe geometry and current component frame available for independent ground contact rays")))return false;
   const auto FrameJson=[](const FTransform& Frame)
   {auto J=S47J::New();Vec(J,TEXT("location_ue_cm"),Frame.GetLocation());Vec(J,TEXT("scale"),Frame.GetScale3D());const FQuat Q=Frame.GetRotation();
    S47J::Set(J,TEXT("rotation_xyzw"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});S47J::Set(J,TEXT("valid"),Frame.IsValid());return J;};
   S47J::Set(Proof,TEXT("actual_body_frame"),FrameJson(C->GetComponentTransform()));
   struct FSurfaceRead
   {
    FJ Json;int32 Index=INDEX_NONE;bool Mapped=false,Registered=false,NodeChain=false,SceneChain=false;
    bool MetaShown=false,NativeHidden=false,AllSceneRegistered=false,Shown=false,Shell=false,CanExclude=false;
   };
   const auto ReadSurface=[&](UPrimitiveComponent* Surface)
   {
    FSurfaceRead Out;Out.Json=S47J::New();auto J=Out.Json;Out.Index=Surface?W->NodeOf(Surface):INDEX_NONE;
    const auto* N=W->Node(Out.Index);auto* Static=Cast<UStaticMeshComponent>(Surface);const auto* SurfaceMesh=Static?Static->GetStaticMesh().Get():nullptr;
    Out.Mapped=Surface&&N&&N->Comp.Get()==Surface&&W->NodeOf(Surface)==Out.Index;Out.Registered=Surface&&Surface->IsRegistered();
    Out.Shell=(N&&N->Name==TEXT("town-production:SM_Town_Laundry_Shell"))||(SurfaceMesh&&SurfaceMesh->GetPathName()==TEXT("/Game/TownBuildings/SM_Town_Laundry_Shell.SM_Town_Laundry_Shell"));
    S47J::Set(J,TEXT("node"),Out.Index);S47J::Set(J,TEXT("component"),GetPathNameSafe(Surface));S47J::Set(J,TEXT("exact_current_node_component_mapping"),Out.Mapped);
    S47J::Set(J,TEXT("registered"),Out.Registered);S47J::Set(J,TEXT("mesh"),GetPathNameSafe(SurfaceMesh));S47J::Set(J,TEXT("same_laundry_shell"),Out.Shell);
    if(N){S47J::Set(J,TEXT("name"),N->Name);S47J::Set(J,TEXT("geo"),N->Geo);S47J::Set(J,TEXT("parent"),N->Parent);S47J::Set(J,TEXT("root"),N->Root);}
    FRows NodeParents;TSet<int32> SeenNodes;Out.NodeChain=Out.Mapped;
    for(int32 I=Out.Index;Out.NodeChain&&I!=INDEX_NONE;)
    {
     const auto* P=W->Node(I);if(!P||SeenNodes.Contains(I)){Out.NodeChain=false;break;}SeenNodes.Add(I);auto R=S47J::New();
     S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("parent"),P->Parent);S47J::Set(R,TEXT("root"),P->Root);S47J::Set(R,TEXT("name"),P->Name);S47J::Set(R,TEXT("node_bVis"),P->bVis);
     S47J::Set(R,TEXT("component"),GetPathNameSafe(P->Comp.Get()));NodeParents.Add(MakeShared<FJsonValueObject>(R));I=P->Parent;
    }
    Out.MetaShown=Out.NodeChain&&W->ObjAt(Out.Index).Shown();J->SetArrayField(TEXT("metadata_ancestors_including_self"),NodeParents);
    S47J::Set(J,TEXT("metadata_chain_valid"),Out.NodeChain);S47J::Set(J,TEXT("metadata_Shown"),Out.MetaShown);
    FRows SceneParents;TSet<const USceneComponent*> SeenScene;Out.SceneChain=Surface!=nullptr;Out.AllSceneRegistered=Surface!=nullptr;
    for(const USceneComponent* P=Surface;P;P=P->GetAttachParent())
    {
     if(SeenScene.Contains(P)){Out.SceneChain=false;break;}SeenScene.Add(P);auto R=S47J::New();const bool OwnerHidden=P->GetOwner()&&P->GetOwner()->IsHidden();
     const bool Hidden=!P->IsVisible()||P->bHiddenInGame||OwnerHidden;Out.NativeHidden=Out.NativeHidden||Hidden;Out.AllSceneRegistered=Out.AllSceneRegistered&&P->IsRegistered();
     S47J::Set(R,TEXT("component"),P->GetPathName());S47J::Set(R,TEXT("registered"),P->IsRegistered());S47J::Set(R,TEXT("is_visible"),P->IsVisible());
     S47J::Set(R,TEXT("hidden_in_game"),bool(P->bHiddenInGame));S47J::Set(R,TEXT("owner_hidden"),OwnerHidden);S47J::Set(R,TEXT("actual_world_transform"),FrameJson(P->GetComponentTransform()));
     SceneParents.Add(MakeShared<FJsonValueObject>(R));
    }
    J->SetArrayField(TEXT("actual_scene_ancestors_including_self"),SceneParents);S47J::Set(J,TEXT("scene_chain_valid"),Out.SceneChain);
    S47J::Set(J,TEXT("all_scene_registered"),Out.AllSceneRegistered);S47J::Set(J,TEXT("actual_native_hidden_flag"),Out.NativeHidden);
    Out.Shown=Out.Mapped&&Out.Registered&&Out.NodeChain&&Out.SceneChain&&Out.AllSceneRegistered&&Out.MetaShown&&!Out.NativeHidden;
    Out.CanExclude=Out.Mapped&&Out.Registered&&Out.NodeChain&&Out.SceneChain&&Out.AllSceneRegistered&&Out.NativeHidden&&!Out.Shell;
    S47J::Set(J,TEXT("actually_shown_registered"),Out.Shown);S47J::Set(J,TEXT("query_local_hidden_exclusion_eligible"),Out.CanExclude);
    if(Surface)
    {S47J::Set(J,TEXT("collision_enabled"),int32(Surface->GetCollisionEnabled()));S47J::Set(J,TEXT("query_collision"),CollisionEnabledHasQuery(Surface->GetCollisionEnabled()));
     S47J::Set(J,TEXT("visibility_response"),int32(Surface->GetCollisionResponseToChannel(ECC_Visibility)));S47J::Set(J,TEXT("actual_world_transform"),FrameJson(Surface->GetComponentTransform()));}
    return Out;
   };
   const auto RecordHit=[&](const FHitResult& H,bool Found,const FVector& Start,const FVector& End)
   {
    auto J=S47J::New();S47J::Set(J,TEXT("hit"),Found);S47J::Set(J,TEXT("raw_blocking_hit"),bool(H.bBlockingHit));S47J::Set(J,TEXT("start_penetrating"),bool(H.bStartPenetrating));
    Vec(J,TEXT("trace_start_ue_cm"),Start);Vec(J,TEXT("trace_end_ue_cm"),End);S47J::Set(J,TEXT("surface_identity"),ReadSurface(H.GetComponent()).Json);
    if(Found)
    {Vec(J,TEXT("actual_impact_ue_cm"),H.ImpactPoint);Vec(J,TEXT("actual_normal_ue"),H.ImpactNormal);S47J::Set(J,TEXT("face_index"),H.FaceIndex);
     S47J::Set(J,TEXT("item"),H.Item);int32 Section=INDEX_NONE;
     UMaterialInterface* Material=H.GetComponent()&&H.FaceIndex!=INDEX_NONE?H.GetComponent()->GetMaterialFromCollisionFaceIndex(H.FaceIndex,Section):nullptr;
     S47J::Set(J,TEXT("collision_material_section"),Section);S47J::Set(J,TEXT("actual_material"),GetPathNameSafe(Material));}
    return J;
   };
   const auto& L=Mesh->GetRenderData()->LODResources[0];
   S47J::Set(Proof,TEXT("actual_LOD0_vertices"),double(L.VertexBuffers.PositionVertexBuffer.GetNumVertices()));S47J::Set(Proof,TEXT("actual_LOD0_triangles"),double(L.GetNumTriangles()));
   if(!ContactCheck(L.VertexBuffers.PositionVertexBuffer.GetNumVertices()>0&&L.VertexBuffers.PositionVertexBuffer.GetVertexData()&&L.IndexBuffer.GetIndexDataSize()>0,
    TEXT("independent sole sampling requires actual resident CPU positions and indices")))return false;
   TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);TSet<uint32> Candidates[2];double Low[2]={1e20,1e20};
   for(const auto& S:L.Sections)
   {
    if(!Mesh->GetStaticMaterials().IsValidIndex(S.MaterialIndex))continue;const auto* Mat=Mesh->GetMaterial(S.MaterialIndex);if(!Mat||(Mat->GetName()!=TEXT("M_RW_Shoe")&&Mat->GetName()!=TEXT("M_RW_Trim")))continue;
    for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)
    {
     if(I>=uint32(Indices.Num())||Indices[I]>=L.VertexBuffers.PositionVertexBuffer.GetNumVertices())
     {S47J::Set(Proof,TEXT("invalid_shoe_section_index"),double(I));if(!ContactCheck(false,TEXT("shoe section index outside actual CPU vertex payload")))return false;}
     const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I]));if(P.Z>=-15)continue;
     const int32 Side=P.Y<0?0:1;Candidates[Side].Add(Indices[I]);Low[Side]=FMath::Min(Low[Side],P.Z);
    }
   }
   if(!ContactCheck(Candidates[0].Num()>=3&&Candidates[1].Num()>=3,TEXT("both shoes have actual lowest-geometry candidates in native Shoe/Trim material sections")))return false;
   for(int32 Side=0;Side<2;++Side)
   {
    int32 Rays=0;double Min=1e20,Max=-1e20;FRows Samples;TArray<FVector> Unique;auto Foot=S47J::New();
    S47J::Set(Foot,TEXT("side"),Side);S47J::Set(Foot,TEXT("ground_rays"),Rays);Feet.Add(MakeShared<FJsonValueObject>(Foot));EvidenceSnapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);
    for(uint32 Index:Candidates[Side])
    {
     const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Index));if(P.Z<=Low[Side]+.15&&!Unique.ContainsByPredicate([&](const FVector& V){return V.Equals(P,.00001);}))Unique.Add(P);
    }
    S47J::Set(Foot,TEXT("unique_lowest_sole_vertices"),Unique.Num());
    if(!ContactCheck(Unique.Num()>=3&&Unique.Num()<=256,TEXT("contact sample includes every unique lowest sole vertex, without duplicate-index or first16 bias")))return false;
    for(const FVector& P:Unique)
    {
     const FVector World=C->GetComponentTransform().TransformPosition(P),Start=World+FVector(0,0,50),End=World-FVector(0,0,100);
     auto R=S47J::New();Vec(R,TEXT("actual_native_local_cm"),P);Vec(R,TEXT("sole_world_ue_cm"),World);S47J::Set(R,TEXT("actual_body_frame"),FrameJson(C->GetComponentTransform()));
     S47J::Set(R,TEXT("entire_laundry_shell_ignored"),false);Samples.Add(MakeShared<FJsonValueObject>(R));Foot->SetArrayField(TEXT("samples"),Samples);
     FRows Queries;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanSole),true);Q.bReturnFaceIndex=true;
     TSet<const UPrimitiveComponent*> Excluded;FHitResult WorldHit;bool FoundVisible=false;FString RejectReason=TEXT("bounded world query exhausted before an eligible visible blocker");
     for(int32 Attempt=0;Attempt<32;++Attempt)
     {
      FHitResult H;const bool Found=SceneWorld->LineTraceSingleByChannel(H,Start,End,ECC_Visibility,Q);auto Query=RecordHit(H,Found,Start,End);
      S47J::Set(Query,TEXT("attempt"),Attempt);S47J::Set(Query,TEXT("query_local_exclusion"),false);S47J::Set(Query,TEXT("already_ignored_component_count"),Excluded.Num());
      Queries.Add(MakeShared<FJsonValueObject>(Query));R->SetArrayField(TEXT("ordered_single_world_queries"),Queries);
      if(!Found||!H.bBlockingHit||H.bStartPenetrating){RejectReason=TEXT("world query lacks a nonpenetrating blocking hit");S47J::Set(Query,TEXT("decision"),RejectReason);break;}
      const auto Surface=ReadSurface(H.GetComponent());S47J::Set(Query,TEXT("surface_identity"),Surface.Json);
      if(Surface.Shell){RejectReason=TEXT("Laundry Shell blocker is retained and cannot be ignored");S47J::Set(Query,TEXT("decision"),RejectReason);break;}
      if(Surface.CanExclude)
      {
       if(Excluded.Contains(H.GetComponent())){RejectReason=TEXT("repeated hidden blocker despite prior local query exclusion");S47J::Set(Query,TEXT("decision"),RejectReason);break;}
       S47J::Set(Query,TEXT("query_local_exclusion"),true);S47J::Set(Query,TEXT("decision"),TEXT("fresh exact mapped registered component has actual native hidden flags; excluded only from this local query"));
       Excluded.Add(H.GetComponent());Q.AddIgnoredComponent(H.GetComponent());continue;
      }
      if(!Surface.Shown){RejectReason=TEXT("unknown, invalid, unregistered or metadata/native-inconsistent blocker cannot be ignored");S47J::Set(Query,TEXT("decision"),RejectReason);break;}
      WorldHit=H;FoundVisible=true;S47J::Set(Query,TEXT("decision"),TEXT("first actual shown registered world blocker retained for independent provider validation"));break;
     }
     S47J::Set(R,TEXT("bounded_world_query_complete"),FoundVisible);S47J::Set(R,TEXT("world_query_count"),Queries.Num());S47J::Set(R,TEXT("conditional_hidden_exclusions"),Excluded.Num());
     if(!FoundVisible)S47J::Set(R,TEXT("rejection_reason"),RejectReason);
     if(!ContactCheck(FoundVisible,TEXT("bounded sole world query must end on an actual visible blocker: ")+RejectReason))return false;
     auto* Ground=Cast<UStaticMeshComponent>(WorldHit.GetComponent());const auto Surface=ReadSurface(Ground);const auto* N=W->Node(1242);const auto* ParentNode=W->Node(1240);
     const auto* GroundMesh=Ground?Ground->GetStaticMesh().Get():nullptr;auto* M=Main();auto Provider=S47J::New();
     const bool Exact=Ground&&N&&Surface.Mapped&&Surface.Index==1242&&N->Comp.Get()==Ground&&N->Geo==873&&N->Parent==1240&&N->Root==1240
      &&GroundMesh&&GroundMesh->GetPathName()==TEXT("/Game/Web/Geo/geo_007/StaticMeshes/g873.g873");
     const bool ParentVisible=Exact&&Surface.NodeChain&&ParentNode&&ParentNode->Name==TEXT("road")&&ParentNode->Comp.IsValid()&&Ground&&Ground->GetAttachParent()==ParentNode->Comp.Get()
      &&ParentNode->Comp->IsRegistered()&&ParentNode->Comp->IsVisible()&&!ParentNode->Comp->bHiddenInGame&&W->ObjAt(1240).Shown()
      &&!(ParentNode->Comp->GetOwner()&&ParentNode->Comp->GetOwner()->IsHidden());
     const bool ActiveArea=M&&M->World&&W->Area()==M->World->Area&&(W->Area()==TEXT("diner")||W->Area()==TEXT("road"));
     const bool QueryBlocks=Ground&&CollisionEnabledHasQuery(Ground->GetCollisionEnabled())&&Ground->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block;
     S47J::Set(Provider,TEXT("actual_surface"),Surface.Json);S47J::Set(Provider,TEXT("exact_1242_g873_parent1240_identity"),Exact);S47J::Set(Provider,TEXT("parent1240_active_and_shown"),ParentVisible);
     S47J::Set(Provider,TEXT("active_road_or_diner_context"),ActiveArea);S47J::Set(Provider,TEXT("query_collision_and_VisibilityBlock"),QueryBlocks);S47J::Set(R,TEXT("independent_provider_validation"),Provider);
     if(!ContactCheck(Exact&&ParentVisible&&ActiveArea&&QueryBlocks&&Surface.Shown&&WorldHit.ImpactNormal.Z>.7,
      TEXT("first visible sole-world hit must be exact shown active road1242/g873 with upward query-blocking surface")))return false;
     FHitResult DirectHit;FCollisionQueryParams DirectQ(SCENE_QUERY_STAT(S47RadioWomanSoleDirect),true);DirectQ.bReturnFaceIndex=true;
     const bool DirectFound=Ground->LineTraceComponent(DirectHit,Start,End,DirectQ);auto Direct=RecordHit(DirectHit,DirectFound,Start,End);
     S47J::Set(Direct,TEXT("requested_exact_component"),Ground->GetPathName());S47J::Set(Direct,TEXT("blocking_flag_is_not_a_direct_API_requirement"),true);S47J::Set(R,TEXT("independent_exact_component_query"),Direct);
     const auto Fresh=ReadSurface(Ground);S47J::Set(R,TEXT("provider_after_direct_query"),Fresh.Json);
     const bool SamePoint=DirectFound&&DirectHit.GetComponent()==Ground&&!DirectHit.bStartPenetrating&&DirectHit.ImpactPoint.Equals(WorldHit.ImpactPoint,.02);
     const bool SameNormal=DirectFound&&!DirectHit.ImpactNormal.ContainsNaN()&&DirectHit.ImpactNormal.Z>.7&&DirectHit.ImpactNormal.Equals(WorldHit.ImpactNormal,.0001);
     S47J::Set(R,TEXT("world_direct_same_component_and_point"),SamePoint);S47J::Set(R,TEXT("world_direct_same_normal"),SameNormal);
     S47J::Set(R,TEXT("maximum_world_direct_point_delta_cm"),.02);S47J::Set(R,TEXT("maximum_world_direct_normal_component_delta"),.0001);
     if(DirectFound){S47J::Set(R,TEXT("actual_world_direct_point_delta_cm"),FVector::Dist(DirectHit.ImpactPoint,WorldHit.ImpactPoint));S47J::Set(R,TEXT("actual_world_direct_normal_delta"),FVector::Dist(DirectHit.ImpactNormal,WorldHit.ImpactNormal));}
     if(!ContactCheck(DirectFound&&SamePoint&&SameNormal&&Fresh.Shown&&!Fresh.Shell,
      TEXT("bool-LineTraceComponent on exactly the shown world-hit SMC must independently confirm point and normal")))return false;
     const double Gap=World.Z-WorldHit.ImpactPoint.Z;Min=FMath::Min(Min,Gap);Max=FMath::Max(Max,Gap);++Rays;
     Vec(R,TEXT("ground_world_ue_cm"),WorldHit.ImpactPoint);S47J::Set(R,TEXT("gap_cm"),Gap);S47J::Set(R,TEXT("ground_component"),Ground->GetPathName());S47J::Set(R,TEXT("sample_complete"),true);
     S47J::Set(Foot,TEXT("ground_rays"),Rays);S47J::Set(Foot,TEXT("minimum_gap_cm"),Min);S47J::Set(Foot,TEXT("maximum_gap_cm"),Max);
    }
    if(!ContactCheck(Rays==Unique.Num()&&Rays>=3&&Min>=-1.5&&Max<=1.5,TEXT("actual seated shoe contact within1.5cm of rendered ground, without body scale or collision changes")))return false;
   }
   S47J::Set(Proof,TEXT("complete"),true);EvidenceSnapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);return true;
  }
  bool Snapshot(const FString& Name,bool Expected)
  {
   auto* M=Main();const auto Woman=Web->Obj(TEXT("world.town.woman")),Proxy=Web->Obj(TEXT("world.town.proxies.woman"));
   if(!Check(Woman.I==1480&&Proxy.I==1544&&M->TownLayer->Town->bWomanIn==Expected&&Woman.Visible()==Expected&&Proxy.Visible()==Expected&&Proxy.Shown()==Expected,TEXT("original woman group/proxy and actual normal TownStep share expected presence: ")+Name))return false;
   auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("clock_seconds"),M->Story->Clock());S47J::Set(O,TEXT("expected_present"),Expected);
   Vec(O,TEXT("player_three_m"),M->Player->Pos);Vec(O,TEXT("proxy_three_m"),Proxy.WorldPosition());
   S47J::Set(O,TEXT("native_readback"),Web->RadioWomanReadback());
   TArray<FString> Lines;const bool Audit=Web->AuditRadioWoman(Lines);S47J::Set(O,TEXT("audit_lines"),Lines);S47J::Set(O,TEXT("audit_pass"),Audit);
   if(Fallback)
   {
    if(!Check(!Audit&&NativeNode==INDEX_NONE,TEXT("explicit Off leaves no mannequin or partial body replacement")))return false;
    if(!Check(S47J::Bool(Web->RadioWomanReadback(),TEXT("fallback_original_bindings_intact")),TEXT("explicit Off leaves all actual original mesh/material/transform/collision bindings intact")))return false;
    for(int32 I=1481;I<=1486;++I)if(!Check(Web->Node(I)&&Web->ObjAt(I).Visible(),TEXT("explicit Off retains each complete original body/radio leaf")))return false;
   }
   else
   {
    if(!Check(Audit&&Body()&&Body()->IsVisible()==Expected&&Web->ObjAt(NativeNode).Shown()==Expected,TEXT("native static body follows existing parent presence and full adapter audit")))return false;
    for(int32 I:{1481,1482,1483,1486})if(!Check(!Web->ObjAt(I).Visible(),TEXT("old body-only leaf remains retired without double body")))return false;
    if(!Check(Web->ObjAt(1484).Visible()&&Web->ObjAt(1485).Visible(),TEXT("original split radio housing and metal panel remain enabled")))return false;
    if(!Check(!Body()->IsComponentTickEnabled()&&Body()->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("posed figure has no animation/tick or extra gameplay blocker")))return false;
    if((Name==TEXT("arrival-present")||Name==TEXT("after-continue-present"))&&!ShoeContact(O))return false;
   }
   if(!Check(Expected?!M->Inter->LabelOf(TEXT("town:woman")).IsEmpty():M->Inter->LabelOf(TEXT("town:woman")).IsEmpty(),TEXT("original woman interaction label agrees with actual presence")))return false;
   const auto Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());S47J::Set(O,TEXT("actual_view_ray_pick"),Pick);
   if(!Fallback&&Name.StartsWith(TEXT("clock-fixture-"))&&!Check(Expected?Pick==TEXT("town:woman"):Pick!=TEXT("town:woman"),TEXT("actual ray can select woman only while original proxy is present")))return false;
   if(!Write(Root/(Name+TEXT(".json")),O)){Result(false,TEXT("snapshot write failed"));return false;}Snapshots.Add(MakeShared<FJsonValueObject>(O));return true;
  }
  void QueuePhoto(const FString& Name){PhotoName=Name;Requested=false;PhotoAt=FPlatformTime::Seconds();}
  bool Photo()
  {
   auto* M=Main();const double Now=FPlatformTime::Seconds();Look(.25);const auto File=Root/(PhotoName+TEXT(".png"));
   if(!Requested){if(Now-PhotoAt<.6)return false;FScreenshotRequest::RequestScreenshot(File,true,false);Requested=true;return false;}
   FJ R;if(!Png(File,R)){if(Now-PhotoAt>12)Result(false,TEXT("complete viewport PNG missing: ")+PhotoName);return false;}
   S47J::Set(R,TEXT("name"),PhotoName);Vec(R,TEXT("player_three_m"),M->Player->Pos);Vec(R,TEXT("eye_ue_cm"),M->Player->EyeUE());Vec(R,TEXT("look_direction_ue"),M->Player->ForwardUE());
   if(auto* PC=Cast<APlayerController>(M->Player->GetController()))if(PC->PlayerCameraManager){S47J::Set(R,TEXT("fov"),PC->PlayerCameraManager->GetFOVAngle());Vec(R,TEXT("camera_ue_cm"),PC->PlayerCameraManager->GetCameraLocation());}
   S47J::Set(R,TEXT("clock_seconds"),M->Story->Clock());S47J::Set(R,TEXT("forced_lod_for_inspection"),Body()?Body()->GetForcedLodModel():0);
   if(!Write(Root/(PhotoName+TEXT(".capture.json")),R)){Result(false,TEXT("capture metadata write failed"));return false;}Photos.Add(MakeShared<FJsonValueObject>(R));return true;
  }
  bool Floor()
  {
   auto* P=Main()->Player.Get();FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanFloor),true,P);
   const bool Hit=Web->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(P->Pos.X,P->FloorY+.12,P->Pos.Z),S47Three::ToUE(P->Pos.X,P->FloorY-.35,P->Pos.Z),ECC_Visibility,Q);
   return Check(P->Walkable(P->Pos.X,P->Pos.Z)&&Hit&&FMath::Abs(S47Three::ToThree(H.ImpactPoint).Y-P->FloorY)<.08,TEXT("ordinary Walker and actual rendered ground at each route waypoint"));
  }
  bool Walk(double Now)
  {
   auto* P=Main()->Player.Get();const double Moved=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();
   if(Moved>FMath::Max(.5,(Now-Last)*8+.1)){Result(false,TEXT("Walker discontinuity; no placement recovery"));return false;}
   WalkMetres+=Moved;LastFoot=P->Pos;Last=Now;
   if(FVector2D(P->Pos.X-ProgressFoot.X,P->Pos.Z-ProgressFoot.Z).Size()>.15){ProgressFoot=P->Pos;ProgressAt=Now;}
   if(Waypoint>=Path.Num()){P->TestMove.Reset();return true;}
   const auto D=Path[Waypoint]-P->Pos;const double Distance=FVector2D(D.X,D.Z).Size();
   if(Distance<.085){P->TestMove.Reset();if(!Floor())return false;++Waypoint;return false;}
   if(Now-ProgressAt>12){P->TestMove.Reset();Result(false,TEXT("ordinary route stalled before bench; no pose or tolerance fallback"));return false;}
   P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=-.08;P->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.5));P->bTestRun=false;return false;
  }
  void Tick()
  {
   const auto KeepAlive=AsShared();if(Done)return;const double Now=FPlatformTime::Seconds();if(Now-Began>180){Result(false,TEXT("targeted journey timeout"));return;}
   auto* M=Main();if(!M||!M->Flow||!M->Player||!M->World||!M->Story||!M->UI||!M->Inter||!M->Evening||!M->TownLayer||!M->TownLayer->Town||!M->TownLayer->Caliche||S47Startup::Blocking()){Ready=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){Ready=0;return;}if(Ready==0){Ready=Now;return;}if(Now-Ready<.6)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Evening->Commute()&&M->World->bDriving,TEXT("ordinary fresh commute before explicit parked-car/clock fixture")))return;
    for(int32 I=0;I<Web->PartCount();++I)if(Web->Node(I)->Name==TEXT("mannequin:radio-woman")){if(!Check(NativeNode==INDEX_NONE,TEXT("exactly one adopted mannequin node")))return;NativeNode=I;}
    const auto Preparation=Web->RadioWomanReadback();Write(Root/TEXT("before-fixtures-preparation.json"),Preparation);
    if(!Fallback&&!Check(S47J::Bool(Preparation,TEXT("preparation_complete"))&&S47J::Bool(Preparation,TEXT("originals_captured"))
     &&(S47J::Bool(Preparation,TEXT("active"))?S47J::Bool(Preparation,TEXT("scene_measurement_after_town_step")):S47J::Bool(Preparation,TEXT("fallback_original_bindings_intact"))),
     TEXT("prepared before test fixtures; earlier road adoption is valid only from actual TownUpdate/current Enter epoch")))return;
    PackBefore=S47J::Obj(S47J::Obj(M->Flow->GameState(),TEXT("evening")),TEXT("pack"));
    auto Car=S47J::New();S47J::Set(Car,TEXT("leg"),TEXT("road"));S47J::Set(Car,TEXT("x"),7994.);S47J::Set(Car,TEXT("y"),0.);S47J::Set(Car,TEXT("z"),2138.);S47J::Set(Car,TEXT("h"),0.);
    S47J::Set(Car,TEXT("scope"),TEXT("only explicit position fixture; no later car or player pose writes"));Write(Root/TEXT("explicit-start-fixture.json"),Car);
    Started=true;M->World->BeginCommute(Car);M->Story->SetClock(-19800);Next(1,TEXT("explicit parked-car fixture at west shoulder and clock18:30; wait ordinary handover"));return;
   }
   if(M->Flow->bStarting||M->World->bBusy)return;
   if(Stage==1)
   {
    if(Now-At<.8)return;if(!Check(M->World->CanGetOut()&&M->World->GetOutHere(),TEXT("ordinary GetOutHere from declared parked commute start")))return;Next(2,TEXT("normal egress; no foot placement"));return;
   }
   if(Stage==2)
   {
    if(Now-At<.8)return;if(!Check(!M->World->bDriving&&M->World->Area==TEXT("diner"),TEXT("actual Caliche walking area after parking")))return;
    if(!Fallback)
    {
     NativeNode=INDEX_NONE;for(int32 I=0;I<Web->PartCount();++I)if(Web->Node(I)->Name==TEXT("mannequin:radio-woman")){if(!Check(NativeNode==INDEX_NONE,TEXT("one adopted native node after actual arrival")))return;NativeNode=I;}
     const auto Live=Web->RadioWomanReadback();
     if(!Check(Body()&&Body()->GetStaticMesh()&&Body()->GetStaticMesh()->GetRenderData()&&Body()->GetStaticMesh()->GetRenderData()->LODResources.Num()==3
      &&S47J::Bool(Live,TEXT("active"))&&S47J::Bool(Live,TEXT("scene_measurement_after_town_step")),TEXT("native active after real GetOutHere/Enter diner and ordinary TownUpdate, never forced by test")))return;
    }
    if(!Snapshot(TEXT("arrival-present"),true)||!Floor())return;
    if(Fallback){Next(11,TEXT("explicit Off: original scene/presence only, no native journey acceptance"));return;}
    const auto T=Target();const auto P=M->Player->Pos;Path={FVector(T.X+2.05,P.Y,P.Z),FVector(T.X+2.05,P.Y,T.Z)};
    for(const auto& Q:Path)if(!Check(M->Player->Walkable(Q.X,Q.Z),TEXT("bench approach remains in original walkable area")))return;
    LastFoot=ProgressFoot=P;Last=ProgressAt=Now;Next(3,TEXT("ordinary continuous Walker approach to original bench proxy"));return;
   }
   if(Stage==3){if(!Walk(Now))return;Look();Body()->SetForcedLodModel(1);QueuePhoto(TEXT("bench-lod0"));Next(4,TEXT("actual close LOD0 pose/radio/seat view"));return;}
   if(Stage==4){if(!Photo())return;Body()->SetForcedLodModel(2);QueuePhoto(TEXT("bench-lod1"));Next(5,TEXT("same actual camera, authored LOD1 inspection"));return;}
   if(Stage==5){if(!Photo())return;Body()->SetForcedLodModel(3);QueuePhoto(TEXT("bench-lod2"));Next(6,TEXT("same actual camera, authored LOD2 inspection"));return;}
   if(Stage==6)
   {
    if(!Photo())return;Body()->SetForcedLodModel(0);Look();Next(14,TEXT("allow actual camera to settle on interaction proxy after inspection"));return;
   }
   if(Stage==14)
   {
    Look();if(Now-At<.35)return;
    if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("town:woman"),TEXT("actual view ray selects unchanged original woman proxy")))return;
    M->Inter->UseId(TEXT("town:woman"));Next(7,TEXT("original interaction opens existing radio-woman conversation"));return;
   }
   if(Stage==7)
   {
    if(Now-At<.5)return;if(!Check(M->UI->Modal()&&M->TownSeen.Contains(TEXT("met:radio_woman")),TEXT("actual original conversation modal and met flag")))return;
    M->TownLayer->Caliche->Choice(TEXT("radio"));QueuePhoto(TEXT("radio-conversation"));Next(8,TEXT("API choice exercises original listening-topic state"));return;
   }
   if(Stage==8)
   {
    if(!Photo())return;if(!Check(M->TownSeen.Contains(TEXT("ask:radio_woman:radio")),TEXT("existing radio-topic flag written by original conversation")))return;
    M->UI->Close();Next(9,TEXT("close actual modal before normal save boundary"));return;
   }
   if(Stage==9)
   {
    if(Now-At<.8)return;if(!Check(WalkMetres>2&&M->Flow->GameSaveBlock().IsEmpty(),TEXT("real bench approach complete and ordinary gameplay save boundary")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();Doors=Web->DoorStates();SeenBefore=M->TownSeen.Array();
    if(!Check(Same(PackBefore,S47J::Obj(S47J::Obj(BeforeSave,TEXT("evening")),TEXT("pack"))),TEXT("bench interaction preserved full commute quest pack")))return;
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("HUM01 radio woman");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;FString Error;
    const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT DISK FIXTURE: full actual case0 snapshot copied to isolated case1: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),BeforeSave),TEXT("fresh reader equals entire actual disk snapshot")))return;
    Write(Root/TEXT("before-continue.disk.json"),BeforeSave);SaveId=Meta->Id;M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(10,TEXT("ordinary title then full Continue lifecycle"));return;
   }
   if(Stage==10)
   {
    if(M->Mode==US47Main::EMode::Title){if(Now-At<.6)return;const auto S=M->Flow->Saves.ContinueSave();if(!Check(S&&S->Id==SaveId,TEXT("ordinary title resolves exact isolated disk save")))return;M->Flow->OpenTestView(TEXT("continue"));return;}
    if(Now-At<1||M->Mode!=US47Main::EMode::Play||!M->Flow->SaveBlock().IsEmpty())return;
    const auto Actual=M->Flow->GameState();const auto A=S47J::Obj(S47J::Obj(BeforeSave,TEXT("evening")),TEXT("car")),B=S47J::Obj(S47J::Obj(Actual,TEXT("evening")),TEXT("car"));
    bool Okay=M->Evening->Commute()&&!M->World->bDriving&&M->World->Area==TEXT("diner")&&M->Player->Pos.Equals(SavedFoot,.015)&&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Story->Clock()-SavedClock)<.001;
    for(const auto& P:Doors)Okay&=Web->DoorStates().FindRef(P.Key)==P.Value;
    for(const auto& K:SeenBefore)Okay&=M->TownSeen.Contains(K);
    for(const TCHAR* K:{TEXT("x"),TEXT("y"),TEXT("z"),TEXT("h")})Okay&=A&&B&&FMath::Abs(S47J::Num(A,K)-S47J::Num(B,K))<.015;
    Okay&=Same(PackBefore,S47J::Obj(S47J::Obj(Actual,TEXT("evening")),TEXT("pack")));
    auto R=S47J::New();S47J::Set(R,TEXT("expected"),BeforeSave);S47J::Set(R,TEXT("actual"),Actual);S47J::Set(R,TEXT("pass"),Okay);Write(Root/TEXT("continue-readback.json"),R);
    if(!Check(Okay,TEXT("full ordinary Flow Continue retained player/car/clock/doors/quest pack and conversation flags"))||!Snapshot(TEXT("after-continue-present"),true))return;
    QueuePhoto(TEXT("after-continue"));Next(12,TEXT("same actual placement after full disk Continue"));return;
   }
   if(Stage==12){if(!Photo())return;Next(11,TEXT("declared clock fixtures exercise actual schedule through ordinary frames"));return;}
   if(Stage==11)
   {
    const double Clocks[]={-21601.,-21600.,17399.,17400.,-19800.};
    if(Schedule<5)
    {
     if(Now-At<.3)return;M->Story->SetClock(Clocks[Schedule]);Look();Next(13,TEXT("explicit clock fixture; wait actual TownStep, never SetPeople"));return;
    }
    if(Fallback){Result(true,TEXT("explicit Off preserves all original body/radio leaves and existing presence/proxy schedule"));return;}
    FString Error;const auto Actual=M->Flow->GameState();const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary Flow.WriteSave after restored schedule: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),Actual),TEXT("fresh reader equals entire normal post-Continue save")))return;
    for(const auto& P:Photos){FJ R;if(!Check(Png(S47J::Str(P->AsObject(),TEXT("file")),R)&&S47J::Str(R,TEXT("sha1"))==S47J::Str(P->AsObject(),TEXT("sha1")),TEXT("final original viewport PNG CRC/IEND/hash verified")))return;}
    Result(true,TEXT("native static three-LOD figure, preserved original radio/proxy, physical bench approach/conversation, actual presence boundaries and full disk/title/Continue"));return;
   }
   if(Stage==13)
   {
    const bool Present[]={false,true,true,false,true};if(Now-At<.35)return;
    if(!Snapshot(FString::Printf(TEXT("clock-fixture-%d"),Schedule),Present[Schedule]))return;++Schedule;Next(11,TEXT("actual schedule readback complete"));return;
   }
  }
 };
}
void S47BeginRadioWomanConversationContract(AS47Web* Web);

void S47BeginRadioWomanContract(AS47Web* Web)
{
 const bool PostRepairRequested=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanBenchPostRepairDiagnostic"));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanBenchDiagnostic"))||PostRepairRequested)
 {
  FString Profile;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Profile);
  if(Profile.Contains(TEXT(".."))||!(Profile.StartsWith(TEXT("/tmp/"))||Profile.StartsWith(TEXT("/var/tmp/"))))
  {UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioBenchDiagnostic REJECTED requires an isolated /tmp profile"));return;}
  const auto S=MakeShared<S47RadioWomanContract::FBenchDiagnostic>();S->Web=Web;S->PostRepair=PostRepairRequested;
   S->Root=FPaths::ProjectSavedDir()/(PostRepairRequested?TEXT("RadioBenchPostRepairDiagnostic"):TEXT("RadioBenchDiagnostic"))/FGuid::NewGuid().ToString(EGuidFormats::Digits);
  if(!IFileManager::Get().MakeDirectory(*S->Root,true)){UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioBenchDiagnostic REJECTED could not create fresh evidence=%s"),*S->Root);return;}
  if(!Web||!Web->GetWorld()){S->Result(true,TEXT("actual Web/world unavailable"));return;}
  Web->GetWorldTimerManager().SetTimer(S->Timer,[S]{S->Tick();},.1f,true);
  UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioBenchDiagnostic armed; wait diner/Play and5 settled seconds, timeout20 evidence=%s"),*S->Root);return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanConversation"))){S47BeginRadioWomanConversationContract(Web);return;}
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanContract")))return;
 FString UserDir,Evening;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 bool Conflict=false;for(const TCHAR* P:{TEXT("S47ProductionViews"),TEXT("S47FarmAccessContract"),TEXT("S47CalicheHomesContract"),TEXT("S47MotelV2Route"),TEXT("S47Title"),TEXT("S47SmokeTest"),TEXT("S47HorrorArrival"),TEXT("S47HorrorContract")})Conflict|=FParse::Param(FCommandLine::Get(),P);
 const bool Fallback=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanContractFallback"));
 if(Conflict||UserDir.Contains(TEXT(".."))||!(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))||!FParse::Value(FCommandLine::Get(),TEXT("S47Evening="),Evening)||Evening!=TEXT("commute")||(Fallback&&!FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanOff"))))
 {UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL requires isolated /tmp profile, commute, no competing driver; fallback requires explicit Off"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto S=MakeShared<S47RadioWomanContract::FControl>();S->Web=Web;S->Fallback=Fallback;
 S->Root=FPaths::ProjectSavedDir()/TEXT("RadioWomanContract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*S->Root,true)){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 Web->GetWorldTimerManager().SetTimer(S->Timer,[S]{S->Tick();},.05f,true);
 UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioWoman contract armed evidence=%s"),*S->Root);
}
