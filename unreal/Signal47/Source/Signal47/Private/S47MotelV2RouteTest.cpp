// Isolated API-driven Walker/Resolve route. Placements and chapter fixture are explicit; no human-input claim.
#include "S47Web.h"
#include "S47MotelV2.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Chapter4.h"
#include "S47TownLayer.h"
#include "S47MotelTv.h"
#include "S47Audio.h"
#include "S47Image.h"
#include "S47Json.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TextureResource.h"
#include "RHICommandList.h"
#include "RHIGPUReadback.h"
#include "RenderingThread.h"
#include "Internationalization/Regex.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MotelRoute,Log,All);
namespace
{
 // Transparent panes deliberately have NoCollision in production. Inspect the
 // actual retained LOD0 render triangles for this visual prop check. Walking
 // floor checks below continue to use the first real physics visibility hit.
 bool RenderedPaneHit(UStaticMeshComponent* C,const FVector& Start,const FVector& End,FVector& Impact)
 {
  UStaticMesh* Mesh=C?C->GetStaticMesh().Get():nullptr;
  if(!C||!C->IsVisible()||C->bHiddenInGame||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
   ||!Mesh||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.IsEmpty())return false;
  const auto& L=Mesh->GetRenderData()->LODResources[0];
  const uint32 Stride=L.VertexBuffers.PositionVertexBuffer.GetStride(),Count=L.VertexBuffers.PositionVertexBuffer.GetNumVertices();
  const uint32 IndexCount=L.IndexBuffer.GetNumIndices(),IndexStride=L.IndexBuffer.Is32Bit()?4:2;
  if(Stride<sizeof(FVector3f)||Count==0||IndexCount==0)return false;
  TArray<uint8> Vertices,Indices;bool Read=false;
  // CPU copies are intentionally discarded by normal assets/cook. Read the
  // retained GPU buffers without changing the asset's CPU-memory policy.
  ENQUEUE_RENDER_COMMAND(S47MotelPaneReadback)([&](FRHICommandListImmediate& Cmd)
  {
   FRHIGPUBufferReadback V(TEXT("S47PanePositions")),I(TEXT("S47PaneIndices"));
   if(!L.VertexBuffers.PositionVertexBuffer.VertexBufferRHI||!L.IndexBuffer.IndexBufferRHI)return;
   V.EnqueueCopy(Cmd,L.VertexBuffers.PositionVertexBuffer.VertexBufferRHI,Stride*Count);
   I.EnqueueCopy(Cmd,L.IndexBuffer.IndexBufferRHI,IndexStride*IndexCount);Cmd.BlockUntilGPUIdle();
   if(!V.IsReady()||!I.IsReady())return;
   const void* VP=V.Lock(Stride*Count);const void* IP=I.Lock(IndexStride*IndexCount);
   if(VP&&IP){Vertices.SetNumUninitialized(Stride*Count);Indices.SetNumUninitialized(IndexStride*IndexCount);FMemory::Memcpy(Vertices.GetData(),VP,Vertices.Num());FMemory::Memcpy(Indices.GetData(),IP,Indices.Num());Read=true;}
   if(VP)V.Unlock();if(IP)I.Unlock();
  });FlushRenderingCommands();
  if(!Read){UE_LOG(LogS47MotelRoute,Error,TEXT("S47: newspaper native GPU buffer readback not ready"));return false;}
  auto Vertex=[&](uint32 N){FVector3f V;FMemory::Memcpy(&V,Vertices.GetData()+N*Stride,sizeof(V));return C->GetComponentTransform().TransformPosition(FVector(V));};
  auto Index=[&](uint32 N){if(IndexStride==4){uint32 I;FMemory::Memcpy(&I,Indices.GetData()+N*4,4);return I;}uint16 I;FMemory::Memcpy(&I,Indices.GetData()+N*2,2);return uint32(I);};
  const FVector D=End-Start;double Nearest=2.;
  for(uint32 I=0;I+2<IndexCount;I+=3)
  {
   const uint32 IA=Index(I),IB=Index(I+1),IC=Index(I+2);if(IA>=Count||IB>=Count||IC>=Count)return false;
   const FVector A=Vertex(IA),B=Vertex(IB),V=Vertex(IC);
   const FVector E1=B-A,E2=V-A,H=FVector::CrossProduct(D,E2);const double Det=FVector::DotProduct(E1,H);if(FMath::Abs(Det)<1e-9)continue;
   const FVector S=Start-A;const double U=FVector::DotProduct(S,H)/Det;if(U<0||U>1)continue;
   const FVector Q=FVector::CrossProduct(S,E1);const double VV=FVector::DotProduct(D,Q)/Det;if(VV<0||U+VV>1)continue;
   const double T=FVector::DotProduct(E2,Q)/Det;if(T>=0&&T<=1&&T<Nearest)Nearest=T;
  }
  if(Nearest>1)return false;Impact=S47Three::ToThree(Start+D*Nearest);return true;
 }
 int32 ChildNamed(AS47Web* W,int32 Root,const FString& Name)
 {
  TArray<int32> Pending{Root};
  while(!Pending.IsEmpty()){const int32 I=Pending.Pop(EAllowShrinking::No);const auto* N=W->Node(I);if(!N)continue;if(N->Name==Name)return I;Pending.Append(N->Children);}
  return INDEX_NONE;
 }
 struct FTelevisionSample
 {
  FString State,Texture;double Picture=0,Sound=0,Level=0,AudioClock=0;int32 Frame=-1;TArray<FColor> Pixels;
 };
 bool TelevisionSample(US47Main* M,const FString& Root,const FString& Name,FTelevisionSample& Out,FString& Error)
 {
  if(!M->TownLayer||!M->TownLayer->MotelTv||!M->Audio||!M->Audio->Ctx){Error=TEXT("live motel TV/audio missing");return false;}
  Out.State=M->TownLayer->MotelTv->ProgrammeState();Out.AudioClock=M->Audio->Ctx->CurrentTime();
  const FRegexPattern Pattern(TEXT("picture ([0-9.]+) s \\(frame ([0-9]+)\\), sound ([0-9.]+) s, level ([0-9.]+)"));
  FRegexMatcher Match(Pattern,Out.State);
  if(!Match.FindNext()){Error=TEXT("staged programme/sound not playing: ")+Out.State;return false;}
  Out.Picture=FCString::Atod(*Match.GetCaptureGroup(1));Out.Frame=FCString::Atoi(*Match.GetCaptureGroup(2));Out.Sound=FCString::Atod(*Match.GetCaptureGroup(3));Out.Level=FCString::Atod(*Match.GetCaptureGroup(4));
  FString Text;if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("WebData/manifest.json")))){Error=TEXT("screen material manifest missing");return false;}
  int32 Material=INDEX_NONE;const int32 Tex=M->Web->HandleTex(TEXT("world.motel.tv.tex"));
  for(const auto& V:S47J::Arr(S47J::Read(Text),TEXT("materials")))if(S47J::Int(V->AsObject(),TEXT("emissiveMap"),-1)==Tex){Material=S47J::Int(V->AsObject(),TEXT("id"),-1);break;}
  auto* Mid=M->Web->SharedMid(Material);UTexture* Image=nullptr;float Enabled=0;
  const int32 Native=ChildNamed(M->Web,M->Web->Handle(TEXT("world.motel.tv.group")),TEXT("Kelvora_OfficeTV"));
  const auto* N=M->Web->Node(Native);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  if(!Mid||!C||!C->IsVisible()||C->GetMaterial(1)!=Mid||!C->GetStaticMesh()||C->GetStaticMesh()->GetName()!=TEXT("SM_Kelvora_TV")||!Mid->GetTextureParameterValue(FMaterialParameterInfo(TEXT("EmissiveMap")),Image)||!Mid->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseEmissiveMap")),Enabled)||Enabled<.99f)
  {Error=TEXT("programme texture is not bound to actual native KELVORA screen slot1");return false;}
  auto* Tube=Cast<UTexture2D>(Image);auto* Resource=Tube?Tube->GetResource():nullptr;
  if(!Resource||!Resource->TextureRHI.IsValid()||Tube->GetSizeX()!=256||Tube->GetSizeY()!=192){Error=TEXT("actual dynamic screen texture unavailable");return false;}
  Out.Texture=Tube->GetPathName();const FTextureRHIRef Texture=Resource->TextureRHI;TArray<FColor> Pixels;
  // Read the texture actually sampled by the native screen, after the queued canvas uploads.
  // UTexture2D's original CPU mip stays unchanged when UpdateTextureRegions uploads later frames.
  ENQUEUE_RENDER_COMMAND(S47MotelRouteScreenRead)([Texture,&Pixels](FRHICommandListImmediate& RHICmdList)
  {RHICmdList.ReadSurfaceData(Texture,FIntRect(0,0,256,192),Pixels,FReadSurfaceDataFlags(RCM_UNorm));});
  FlushRenderingCommands();Out.Pixels=MoveTemp(Pixels);
  if(Out.Pixels.Num()!=256*192||Out.Level<=0){Error=TEXT("native screen readback or programme speaker gain invalid");return false;}
  const FString Url=S47Image::ToDataUrl(Out.Pixels,256,192,false);TArray<uint8> Bytes;const int32 Comma=Url.Find(TEXT(","));
  if(Comma<0||!FBase64::Decode(Url.Mid(Comma+1),Bytes)||!FFileHelper::SaveArrayToFile(Bytes,*(Root/(Name+TEXT(".png"))))){Error=TEXT("screen readback evidence write failed");return false;}
  return true;
 }
 bool PoolExteriorSample(US47Main* M,const FString& Root,FString& Error)
 {
  auto* W=M->Web.Get();auto* P=M->Player.Get();auto O=S47J::New();
  S47J::Set(O,TEXT("method"),TEXT("Visible native/legacy mesh bounds and actual ISM instance world bounds within3m of each exterior fence corner, plus actual complex visibility rays from the uninterrupted Walker route. Proximity uses the nearest XZ point of each AABB; instance bounds transform the mesh-local box, excluding shader sway."));
  S47J::Set(O,TEXT("limits"),TEXT("Large joined-mesh bounds can include distant islands. A fence hit can occlude the small dark E04/E05 candidate. These data identify candidates; they do not identify or authorize removing the brown pump housing or an unknown triangle."));
  S47J::Set(O,TEXT("eye_three_m"),TArray<double>{P->Pos.X,P->FloorY+P->Eye,P->Pos.Z});
  static const FVector2D Corners[]={{-64.9,665.9},{-53.9,665.9},{-64.9,682.9},{-53.9,682.9}};
  TArray<TSharedPtr<FJsonValue>> Meshes,Rays,Instances;
  for(int32 I=0;I<W->PartCount();++I)
  {
   const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
   if(!C||!C->GetStaticMesh()||!C->IsVisible()||C->bHiddenInGame||!FS47Obj(W,I).Shown())continue;
   const FBox U=C->Bounds.GetBox();if(!U.IsValid)continue;
   const FVector Min=S47Three::ToThree(U.Min),Max=S47Three::ToThree(U.Max);
   if(Max.Y<-2.9||Min.Y>1.5)continue;
   TArray<double> Near;
   for(int32 K=0;K<UE_ARRAY_COUNT(Corners);++K)
   {
    const double Dx=FMath::Max(FMath::Max(Min.X-Corners[K].X,0.),Corners[K].X-Max.X),Dz=FMath::Max(FMath::Max(Min.Z-Corners[K].Y,0.),Corners[K].Y-Max.Z);
    if(Dx*Dx+Dz*Dz<=9)Near.Add(K);
   }
   if(Near.IsEmpty())continue;
   auto R=S47J::New();S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("parent"),N->Parent);S47J::Set(R,TEXT("geo"),N->Geo);S47J::Set(R,TEXT("name"),N->Name);S47J::Set(R,TEXT("mesh"),C->GetStaticMesh()->GetPathName());
   S47J::Set(R,TEXT("min_three_m"),TArray<double>{Min.X,Min.Y,Min.Z});S47J::Set(R,TEXT("max_three_m"),TArray<double>{Max.X,Max.Y,Max.Z});S47J::Set(R,TEXT("near_corner_indices"),Near);
   S47J::Set(R,TEXT("joined_bounds_not_local_island"),Max.X-Min.X>8||Max.Z-Min.Z>8);Meshes.Add(MakeShared<FJsonValueObject>(R));
  }
  // Enumerate actual owned components, including flora's second paired draw which has no web node.
  // NoCollision instances cannot answer the visibility rays; inspect their real transforms instead.
  TInlineComponentArray<UInstancedStaticMeshComponent*> Isms;W->GetComponents(Isms);
  for(auto* C:Isms)
  {
   if(!C||!C->IsRegistered()||!C->GetStaticMesh()||!C->IsVisible()||C->bHiddenInGame)continue;
   const int32 I=W->NodeOf(C);const auto* N=W->Node(I);bool Shown=I<0||FS47Obj(W,I).Shown();
   for(auto* Parent=C->GetAttachParent();Parent&&Shown;Parent=Parent->GetAttachParent())Shown=Parent->IsVisible()&&!Parent->bHiddenInGame;
   if(!Shown)continue;
   const FBox Local=C->GetStaticMesh()->GetBoundingBox();if(!Local.IsValid)continue;
   for(int32 Index=0;Index<C->GetInstanceCount();++Index)
   {
    FTransform World;if(!C->GetInstanceTransform(Index,World,true))continue;
    const FBox Box=Local.TransformBy(World);if(!Box.IsValid||Box.GetSize().IsNearlyZero())continue;
    const FVector Min=S47Three::ToThree(Box.Min),Max=S47Three::ToThree(Box.Max);TArray<double> Near,Distances;
    for(int32 K=0;K<UE_ARRAY_COUNT(Corners);++K)
    {
     const double Dx=FMath::Max(FMath::Max(Min.X-Corners[K].X,0.),Corners[K].X-Max.X),Dz=FMath::Max(FMath::Max(Min.Z-Corners[K].Y,0.),Corners[K].Y-Max.Z);
     Distances.Add(FMath::Sqrt(Dx*Dx+Dz*Dz));if(Dx*Dx+Dz*Dz<=9)Near.Add(K);
    }
    if(Near.IsEmpty())continue;
    const FVector Translation=World.GetTranslation(),Scale=World.GetScale3D(),Origin=S47Three::ToThree(Translation),Center=S47Three::ToThree(Box.GetCenter());const FQuat Rotation=World.GetRotation();
    auto Transform=S47J::New();S47J::Set(Transform,TEXT("translation_cm"),TArray<double>{Translation.X,Translation.Y,Translation.Z});S47J::Set(Transform,TEXT("rotation_xyzw"),TArray<double>{Rotation.X,Rotation.Y,Rotation.Z,Rotation.W});S47J::Set(Transform,TEXT("scale_xyz"),TArray<double>{Scale.X,Scale.Y,Scale.Z});
    auto R=S47J::New();S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("name"),N?N->Name:TEXT("unregistered"));S47J::Set(R,TEXT("component"),C->GetPathName());S47J::Set(R,TEXT("mesh"),C->GetStaticMesh()->GetPathName());S47J::Set(R,TEXT("index"),Index);
    S47J::Set(R,TEXT("collision_enabled"),int32(C->GetCollisionEnabled()));S47J::Set(R,TEXT("no_collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);S47J::Set(R,TEXT("world_transform_ue"),Transform);
    S47J::Set(R,TEXT("origin_three_m"),TArray<double>{Origin.X,Origin.Y,Origin.Z});S47J::Set(R,TEXT("center_three_m"),TArray<double>{Center.X,Center.Y,Center.Z});S47J::Set(R,TEXT("min_three_m"),TArray<double>{Min.X,Min.Y,Min.Z});S47J::Set(R,TEXT("max_three_m"),TArray<double>{Max.X,Max.Y,Max.Z});
    S47J::Set(R,TEXT("near_corner_indices"),Near);S47J::Set(R,TEXT("distance_to_corner_xz_m"),Distances);Instances.Add(MakeShared<FJsonValueObject>(R));
   }
  }
  for(int32 K=0;K<UE_ARRAY_COUNT(Corners);++K)for(const double Height:{.15,.55})
  {
   // Aim just outside all four corners: the screenshot's right-hand post depends on its view.
   const FVector Target(Corners[K].X+(K%2? .35:-.35),-2.0985683853163195+Height,Corners[K].Y+(K<2?-.35:.35));
   FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MotelRoutePoolExterior),true,P);Q.bReturnFaceIndex=true;
   const bool Hit=W->GetWorld()->LineTraceSingleByChannel(H,P->EyeUE(),S47Three::ToUE(Target),ECC_Visibility,Q);
   auto R=S47J::New();S47J::Set(R,TEXT("corner"),K);S47J::Set(R,TEXT("target_three_m"),TArray<double>{Target.X,Target.Y,Target.Z});S47J::Set(R,TEXT("hit"),Hit);
   if(Hit)
   {
    const int32 I=W->NodeOf(H.GetComponent());const auto* N=W->Node(I);auto* C=Cast<UStaticMeshComponent>(H.GetComponent());const FVector Impact=S47Three::ToThree(H.ImpactPoint);int32 Section=INDEX_NONE;
    auto* Material=C?C->GetMaterialFromCollisionFaceIndex(H.FaceIndex,Section):nullptr;
    S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("name"),N?N->Name:TEXT("unregistered"));S47J::Set(R,TEXT("mesh"),C&&C->GetStaticMesh()?C->GetStaticMesh()->GetPathName():FString());
    S47J::Set(R,TEXT("impact_three_m"),TArray<double>{Impact.X,Impact.Y,Impact.Z});S47J::Set(R,TEXT("face"),H.FaceIndex);S47J::Set(R,TEXT("section"),Section);S47J::Set(R,TEXT("material"),GetPathNameSafe(Material));
   }
   Rays.Add(MakeShared<FJsonValueObject>(R));
  }
  O->SetArrayField(TEXT("meshes"),Meshes);O->SetArrayField(TEXT("rays"),Rays);O->SetArrayField(TEXT("instances"),Instances);
  if(Meshes.IsEmpty()||!FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("pool-exterior.json")))){Error=TEXT("pool exterior candidate/bounds evidence unavailable");return false;}
  UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 pool exterior diagnosis meshes=%d instances=%d rays=%d evidence=%s; unknown candidate retained"),Meshes.Num(),Instances.Num(),Rays.Num(),*(Root/TEXT("pool-exterior.json")));return true;
 }
}
void AS47Web::BeginMotelV2Route()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47MotelV2Route")))return;
 // Action: 1=TV hold, 2=physical payphone, 3=newspaper front, 4=guest-door recess, 5=enter room6, 6=pool exterior diagnosis.
 struct FPoint {double X,Z;const TCHAR* Proxy;const TCHAR* Id;int32 Action=0;};
 static const FPoint Route[]={
  {-44.8,674.1,TEXT("register"),TEXT("court:register")},
  {-44.8,673,nullptr,nullptr},{-46.1,673,TEXT("keyBoard"),TEXT("court:keyBoard")},
  {-45.4,673,TEXT("officePhone"),TEXT("court:officePhone")},
  {-44.8,673,nullptr,nullptr},{-45.5,674.1,nullptr,nullptr},
  {-45.5,675.8,TEXT("officeTv"),TEXT("court:officeTv"),1},
  {-45.5,674.1,nullptr,nullptr},{-44.8,674.1,nullptr,nullptr},
  {-42,674.1,nullptr,nullptr},{-42,671,nullptr,nullptr},
  {-42.3,671,nullptr,nullptr},{-42.3,670.35,nullptr,nullptr},
  {-43.25,670.35,nullptr,nullptr,2},{-44.7,670.35,TEXT("iceMachine"),TEXT("court:iceMachine")},
  {-46,670.35,nullptr,nullptr,3},{-53.2,670.55,nullptr,nullptr},
  {-53.2,680.6,nullptr,nullptr,6},{-55.5,680.6,nullptr,nullptr},
  {-55.5,667.3,nullptr,nullptr},{-64.2,667.3,nullptr,nullptr},
  {-64.2,679.5,nullptr,nullptr},{-55.5,679.5,nullptr,nullptr},{-55.5,680.6,nullptr,nullptr},
  {-53.2,680.6,nullptr,nullptr},{-53.2,670.55,nullptr,nullptr},
  {-42.3,670.55,nullptr,nullptr},{-42.3,671,nullptr,nullptr},{-42,671,nullptr,nullptr},
  {-42,674.1,nullptr,nullptr},{-42.25,674.1,nullptr,nullptr},{-42.25,680,nullptr,nullptr},
  // Approach each door from the continuous aisle, then retreat before passing the next chair/pot.
  {-41.9,680,nullptr,nullptr},{-43.15,680,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,680,nullptr,nullptr},
  {-41.9,685,nullptr,nullptr},{-43.15,685,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,685,nullptr,nullptr},
  {-41.9,690,nullptr,nullptr},{-43.15,690,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,690,nullptr,nullptr},
  {-41.9,695,nullptr,nullptr},{-43.15,695,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,695,nullptr,nullptr},
  {-41.9,700,nullptr,nullptr},{-43.15,700,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,700,nullptr,nullptr},
  {-41.9,705,nullptr,nullptr},{-43.15,705,TEXT("room6Door"),TEXT("room6Door")},{-41.9,705,nullptr,nullptr},
  {-41.9,710,nullptr,nullptr},{-43.15,710,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,710,nullptr,nullptr},
  {-41.9,715,nullptr,nullptr},{-43.15,715,TEXT("otherDoors"),TEXT("court:otherDoors"),4},{-41.9,715,nullptr,nullptr},
  {-41.9,705,nullptr,nullptr},{-43.15,705,TEXT("room6Door"),TEXT("room6Door"),5}
 };
 struct FState
 {
  bool Started=false,Done=false;int32 Stage=0,Index=0,Probe=0,Picks=0,FloorRays=0,Recesses=0;double Since=0,Began=0,Reached=0,Distance=0,TvSince=0,SavedClock=0;
  FVector Last=FVector::ZeroVector,ProbeStart=FVector::ZeroVector;FS47Rect Box;FString Root;FTelevisionSample TvA,TvB;S47J::FObj Saved;
 };
 const auto S=MakeShared<FState>();const TWeakObjectPtr<AS47Web> Self=this;FTimerHandle Timer;
 GetWorldTimerManager().SetTimer(Timer,[Self,S]
 {
  if(S->Done)return;auto* W=Self.Get();auto* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->World||!M->Flow||!M->Player||!M->Inter||!M->Story||!M->UI)return;
  auto* P=M->Player.Get();const double Now=FPlatformTime::Seconds();
  auto Result=[&](const FString& Status,const FString& Why)
  {
   if(S->Root.IsEmpty())return;auto O=S47J::New();S47J::Set(O,TEXT("status"),Status);S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("waypoints"),S->Index);S47J::Set(O,TEXT("interactions"),S->Picks);S47J::Set(O,TEXT("doorRecesses"),S->Recesses);S47J::Set(O,TEXT("floorRays"),S->FloorRays);S47J::Set(O,TEXT("walkedMetres"),S->Distance);
   S47J::Set(O,TEXT("tvBefore"),S->TvA.State);S47J::Set(O,TEXT("tvAfter"),S->TvB.State);S47J::Set(O,TEXT("tvTexture"),S->TvB.Texture);S47J::Set(O,TEXT("elapsedSeconds"),Now-S->Began);
   S47J::Set(O,TEXT("tvAudioClockDelta"),S->TvB.AudioClock-S->TvA.AudioClock);S47J::Set(O,TEXT("poolExteriorDiagnostic"),TEXT("pool-exterior.json"));
   S47J::Set(O,TEXT("method"),TEXT("API-driven normal Walker/Resolve; four explicit blocker placements then one route-start placement; actual interaction rays and room6Door use; native TV texture GPU readback and production programme/audio clocks; ordinary isolated disk save/title/Continue."));
   S47J::Set(O,TEXT("limits"),TEXT("Legacy chapter4 entry is an explicit developer fixture. Earlier chapters, human input, subjective listening and full chapter4 content were not played."));
   FFileHelper::SaveStringToFile(S47J::Write(O,true),*(S->Root/TEXT("result.json")));
  };
  auto Fail=[&](const FString& Why){P->TestMove.Reset();P->bTestRun=false;S->Done=true;Result(TEXT("FAIL"),Why);UE_LOG(LogS47MotelRoute,Error,TEXT("S47: motel V2 route FAIL stage=%d point=%d probe=%d at %.3f %.3f: %s"),S->Stage,S->Index,S->Probe,P->Pos.X,P->Pos.Z,*Why);FPlatformMisc::RequestExitWithStatus(false,1);};
  if(!S->Started)
  {
   if(M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
   FString UserDir,Jump;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
   if(!M->Flow->bDevRun||M->Flow->CaseId!=0||Jump!=TEXT("chapter4")||!(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir))
   {Fail(TEXT("requires isolated /tmp UserDir, case0 developer run and explicit S47Jump=chapter4 fixture"));return;}
   FString Other;for(const TCHAR* Flag:{TEXT("S47MotelTv"),TEXT("S47Ch4Drive"),TEXT("S47HorrorContract"),TEXT("S47T01Contract"),TEXT("S47EveningRoute")})if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Other)){Fail(TEXT("do not combine independent live drivers"));return;}
   if(!W->MotelV2||M->World->bDriving||!M->Story->Begun.Contains(TEXT("ch4"))){Fail(TEXT("requires installed motel V2, active chapter4 fixture and on-foot start"));return;}
   TArray<FString> Lines;if(!W->MotelV2->Audit(Lines)){Fail(TEXT("installation audit"));return;}
   S->Root=FPaths::ProjectSavedDir()/TEXT("MotelV2Route")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*S->Root,true);
   M->World->Enter(TEXT("saro"));S->Started=true;S->Since=S->Began=Now;
   UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route START evidence=%s; chapter4 entry fixture; four isolated blocker placements followed by one continuous Walker route; no human-input claim"),*S->Root);return;
  }
  if(Now-S->Began>420){Fail(TEXT("overall timeout"));return;}
  if(S->Stage==4)
  {
   if(Now-S->Since<.5)return;
   if(M->Mode!=US47Main::EMode::Title||!M->Flow->OpenTestView(TEXT("continue"))){Fail(TEXT("ordinary title Continue unavailable"));return;}
   S->Stage=5;S->Since=Now;return;
  }
  if(M->Flow->bStarting||S47Startup::Blocking())return;
  if(S->Stage==5)
  {
   if(M->World->bBusy||Now-S->Since<1)return;
   const auto Pose=S47J::Obj(S->Saved,TEXT("pose"));auto* Chapter=Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));
   const bool Restored=M->Mode==US47Main::EMode::Play&&M->World->Area==TEXT("room6")&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&Chapter&&Chapter->S.bArrived&&Chapter->S.Stage==TEXT("room")&&FMath::Abs(M->Story->Clock()-S->SavedClock)<3&&FMath::Abs(P->Pos.X-S47J::Num(Pose,TEXT("x")))<.03&&FMath::Abs(P->Pos.Z-S47J::Num(Pose,TEXT("z")))<.03;
   TArray<FString> Lines;if(!Restored||!W->MotelV2->Audit(Lines)||S->Recesses!=7||S->Picks!=14){Fail(TEXT("full disk/title Continue changed room6 state, clock, pose or motel installation; all route interactions must have passed"));return;}
   S->Done=true;Result(TEXT("PASS"),TEXT("continuous motel route, physical room6 use and ordinary disk Continue"));UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route PASS waypoints=%d interactions=%d doorRecesses=%d/7 floorRays=%d blockers=4/4 walked=%.2fm elapsed=%.1fs diskContinue=PASS; API-driven Walker, human input unverified"),S->Index,S->Picks,S->Recesses,S->FloorRays,S->Distance,Now-S->Began);FPlatformMisc::RequestExit(false);return;
  }
  if(M->Mode!=US47Main::EMode::Play)return;
  if(S->Stage==0)
  {
   // Independent collision probes precede, and cannot teleport around, the continuous player route.
   if(S->Probe<8)
   {
    const int32 Kind=S->Probe/2;
    if(S->Probe%2==0)
    {
     bool Found=false;for(const auto& B:W->CollidersMutable(TEXT("saro")))
     {
      const bool Match=Kind==0?(FMath::Abs(B.MinX+64.96)<.02&&FMath::Abs(B.MaxX+64.84)<.02):Kind==1?(FMath::Abs(B.MinX+61)<.02&&FMath::Abs(B.MaxX+56.8)<.02):Kind==2?B.Id==TEXT("prop:motelCar1"):(FMath::Abs(B.MinX+48.2)<.02&&FMath::Abs(B.MaxX+43.55)<.02);
      if(B.bEnabled&&Match){S->Box=B;Found=true;break;}
     }
     if(!Found){Fail(TEXT("preserved collision fixture rectangle missing"));return;}
     const double Z=Kind==0?674.9:Kind==3?700:(S->Box.MinZ+S->Box.MaxZ)*.5;
     P->Place(S->Box.MaxX+.8,Z,UE_DOUBLE_PI/2);S->ProbeStart=P->Pos;S->Since=Now;P->TestMove=FVector2D(0,-1);++S->Probe;return;
    }
    if(Now-S->Since<2)return;P->TestMove.Reset();const double Moved=S->ProbeStart.X-P->Pos.X;
    if(Moved<.1||Moved>1.1||P->Pos.X<S->Box.MaxX+P->Radius-.05){Fail(TEXT("normal Walker crossed preserved blocker or did not approach it"));return;}
    UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route blocker PASS kind=%d moved=%.3f edge=%.3f"),Kind,Moved,P->Pos.X);++S->Probe;return;
   }
   P->Place(Route[0].X,Route[0].Z,0);P->Pitch=-.06;S->Last=P->Pos;S->Stage=1;S->Since=Now;return;
  }
  if(S->Stage==3)
  {
   if(M->World->bBusy){if(Now-S->Since>10)Fail(TEXT("room6 transition stalled"));return;}
   if(M->World->Area!=TEXT("room6")){Fail(TEXT("ordinary room6Door use did not enter room6"));return;}
   auto* Chapter=Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));if(!Chapter||!Chapter->S.bArrived||Chapter->S.Stage!=TEXT("room")){Fail(TEXT("actual room6 arrival state missing"));return;}
   // Quarantine before querying the ordinary save gate: a developer run correctly refuses saves.
   if(M->Flow->bDevRun)
   {
    M->Flow->Saves=FS47SaveStore(S->Root/TEXT("SaveGames"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);
   }
   if(!M->Flow->SaveBlock().IsEmpty()){if(Now-S->Since>10)Fail(TEXT("room6 did not reach ordinary save boundary: ")+M->Flow->SaveBlock());return;}
   S->Saved=M->Flow->GameState();S->SavedClock=M->Story->Clock();FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);
   FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Choice=Reader.ContinueSave();const auto Disk=Choice?Reader.Load(Choice->Id):nullptr;
   if(!Meta||!Choice||!Disk||Choice->Id!=Meta->Id||S47J::Str(Disk,TEXT("area"))!=TEXT("room6")){Fail(TEXT("ordinary saved room6/Continue record missing: ")+Error);return;}
   FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(S->Root/TEXT("room6.saved.json")));S->Saved=Disk;M->Flow->ToTitle();S->Stage=4;S->Since=Now;return;
  }
  if(M->World->bBusy||M->UI->Modal())return;
  const double Step=FVector2D(P->Pos.X-S->Last.X,P->Pos.Z-S->Last.Z).Size();if(Step>.65){Fail(TEXT("continuous route moved farther than one native walking step"));return;}S->Distance+=Step;S->Last=P->Pos;
  const FPoint& Goal=Route[S->Index];const double Dx=Goal.X-P->Pos.X,Dz=Goal.Z-P->Pos.Z,Distance=FMath::Sqrt(Dx*Dx+Dz*Dz);
  if(Distance>=.08)
  {
   S->Reached=0;if(Now-S->Since>24){Fail(TEXT("waypoint stalled; collision must not be bypassed"));return;}
   P->Yaw=FMath::Atan2(-Dx,-Dz);P->TestMove=FVector2D(0,-FMath::Clamp(Distance/.5,0.,1.));P->bTestRun=false;return;
  }
  P->TestMove.Reset();if(!S->Reached){S->Reached=Now;return;}if(Now-S->Reached<.25)return;
  const double Floor=FS47MotelV2::FloorAt(P->Pos.X,P->Pos.Z);
  if(!FMath::IsFinite(Floor)||FMath::Abs(P->FloorY-Floor)>.012){Fail(TEXT("Walker floor does not match authored surface height"));return;}
  int32 GroundHits=0;TArray<FString> FloorSamples;
  // Five foot samples allow a genuine2cm concrete joint, but reject an invisible raised plane.
  for(const FVector2D& Offset:{FVector2D(0,0),FVector2D(.035,0),FVector2D(-.035,0),FVector2D(0,.035),FVector2D(0,-.035)})
  {
   FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MotelRouteFloor),true,P);
   const bool Hit=W->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(P->Pos.X+Offset.X,Floor+.025,P->Pos.Z+Offset.Y),S47Three::ToUE(P->Pos.X+Offset.X,Floor-.2,P->Pos.Z+Offset.Y),ECC_Visibility,Q);
   if(Hit)
   {
    const auto* N=W->Node(W->NodeOf(H.GetComponent()));if(N&&N->Name.StartsWith(TEXT("motel-v2:"))&&FMath::Abs(S47Three::ToThree(H.ImpactPoint).Y-Floor)<.015)++GroundHits;
    FloorSamples.Add(FString::Printf(TEXT("offset=%.3f,%.3f hit=%s node=%s height=%.6f expected=%.6f"),Offset.X,Offset.Y,*GetNameSafe(H.GetComponent()),N?*N->Name:TEXT("unregistered"),S47Three::ToThree(H.ImpactPoint).Y,Floor));
   }
   else FloorSamples.Add(FString::Printf(TEXT("offset=%.3f,%.3f no collision hit, expected height=%.6f"),Offset.X,Offset.Y,Floor));
  }
  if(GroundHits<3){for(const auto& Line:FloorSamples)UE_LOG(LogS47MotelRoute,Error,TEXT("S47: motel V2 floor diagnosis %s"),*Line);Fail(FString::Printf(TEXT("native visible walking surface absent under feet: %d/5 rays"),GroundHits));return;}S->FloorRays+=5;
  if(Goal.Action==6){FString Error;if(!PoolExteriorSample(M,S->Root,Error)){Fail(Error);return;}}
  if(Goal.Action==2||Goal.Action==3)
  {
   const bool Phone=Goal.Action==2;const FVector Target=Phone?FVector(-43.25,-2.0985683853163195+1.11,671.22):FVector(-46,-2.0985683853163195+.6,671.035);
   FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MotelRouteProp),true,P);
   const FVector Eye=S47Three::ToThree(P->EyeUE());
   const FVector End=S47Three::ToUE(Target+(Target-Eye).GetSafeNormal()*.3);UStaticMeshComponent* Pane=nullptr;
   if(!Phone)for(int32 I=0;I<W->PartCount();++I)if(const auto* Part=W->Node(I);Part&&Part->Name==TEXT("motel-v2:SM_Sierra_Glass"))Pane=Cast<UStaticMeshComponent>(Part->Comp.Get());
   FVector Impact=FVector::ZeroVector;
   const bool Hit=Phone?W->GetWorld()->LineTraceSingleByChannel(H,P->EyeUE(),End,ECC_Visibility,Q):RenderedPaneHit(Pane,P->EyeUE(),End,Impact);
   const auto* N=Hit?W->Node(W->NodeOf(Phone?H.GetComponent():Pane)):nullptr;
   if(Phone&&Hit)Impact=S47Three::ToThree(H.ImpactPoint);
   // The newspaper's moved pane is authored in SM_Sierra_Glass. Its actual
   // FBX ray intersects the pane at local By=3.8595 before the cabinet body.
   // Bind this surface to its measured pane, rather than another Office face.
   const bool NewspaperPane=N&&N->Name==TEXT("motel-v2:SM_Sierra_Glass")
    &&Impact.X>=-46.208&&Impact.X<=-45.792
    &&Impact.Y>=-2.0985683853163195+.482&&Impact.Y<=-2.0985683853163195+.978
    &&Impact.Z>=671.0225&&Impact.Z<=671.0435;
   if(!N||(Phone?N->Name!=TEXT("prop:motelPhone"):!NewspaperPane)||FVector::Distance(Impact,Target)>(Phone?.25:.03))
   {
    UE_LOG(LogS47MotelRoute,Error,TEXT("S47: motel V2 prop diagnosis kind=%s hit=%d component=%s node=%s impact=(%.6f,%.6f,%.6f) target=(%.6f,%.6f,%.6f) delta=%.6fm"),Phone?TEXT("payphone-physics"):TEXT("newspaper-render-geometry"),Hit,*GetNameSafe(Phone?H.GetComponent():Pane),N?*N->Name:TEXT("none"),Impact.X,Impact.Y,Impact.Z,Target.X,Target.Y,Target.Z,FVector::Distance(Impact,Target));
    Fail(TEXT("physical payphone/newspaper front ray missed actual prop"));return;
   }
   if(Phone)
   {
    const FVector FrontUE=H.GetComponent()->GetComponentQuat().RotateVector(S47Three::DirToUE(FVector(0,0,1)));
    const FVector Front(FrontUE.X,FrontUE.Z,FrontUE.Y),At=S47Three::ToThree(H.GetComponent()->GetComponentLocation());
    if(FVector::DotProduct(P->Pos-At,Front)<.6){Fail(TEXT("payphone approached from back rather than usable front"));return;}
   }
   UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route prop PASS %s distance=%.3fm; no new product interaction"),Phone?TEXT("payphone physics front ray"):TEXT("visible NoCollision newspaper pane LOD0 render triangles"),FVector::Distance(Eye,Impact));
  }
  if(Goal.Proxy&&!(Goal.Action==1&&S->TvSince>0))
  {
   int32 Node=W->Handle(FString(TEXT("world.motel.proxies."))+Goal.Proxy);
   if(Goal.Action==4)if(const auto* Group=W->Node(Node))for(const int32 Child:Group->Children)if(FMath::Abs(W->WorldThree(Child).Z-Goal.Z)<.02){Node=Child;break;}
   if(!W->Node(Node)||!M->Inter->Get(Goal.Id)){Fail(FString(TEXT("missing live interaction "))+Goal.Id);return;}
   const FVector Target=W->WorldUE(Node);const FString Hit=M->Inter->Pick(P->EyeUE(),Target-P->EyeUE(),3.6);
   if(Hit!=Goal.Id){Fail(FString::Printf(TEXT("actual interaction ray %s returned '%s'"),Goal.Id,*Hit));return;}
   ++S->Picks;if(Goal.Action==4)++S->Recesses;
   if(Goal.Action==5)
   {
    M->Inter->UseId(Goal.Id);S->Index++;S->Stage=3;S->Since=Now;return;
   }
   if(FString(Goal.Id)==TEXT("court:iceMachine"))M->Inter->UseId(Goal.Id);
  }
  if(Goal.Action==1)
  {
   const FVector Screen=FS47Obj(W,W->Handle(TEXT("world.motel.tv.group"))).LocalToWorld(FVector(-.045,.18,.1815)),D=Screen-FVector(P->Pos.X,P->FloorY+P->Eye,P->Pos.Z);
   P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   FString Error;
   if(!S->TvSince){if(!TelevisionSample(M,S->Root,TEXT("tv-before"),S->TvA,Error)){Fail(Error);return;}S->TvSince=Now;return;}
   if(Now-S->TvSince<6)return;
   if(!TelevisionSample(M,S->Root,TEXT("tv-after"),S->TvB,Error)){Fail(Error);return;}
   int32 Changed=0;for(int32 I=0;I<S->TvA.Pixels.Num();++I)if(S->TvA.Pixels[I]!=S->TvB.Pixels[I])++Changed;
   const double Pic=S->TvB.Picture-S->TvA.Picture,Sound=S->TvB.Sound-S->TvA.Sound;
   if(S->TvB.Texture!=S->TvA.Texture||S->TvB.Frame==S->TvA.Frame||Pic<4.5||Sound<4.5||FMath::Abs(Pic-Sound)>.3||S->TvB.AudioClock-S->TvA.AudioClock<4.5||Changed<256){Fail(TEXT("native bound movie pixels, frames and speaker clock did not advance together"));return;}
   UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route TV PASS native material slot1 assigned; pixels changed=%d/49152; before=%s; after=%s; audioClockDelta=%.3f; no subjective listening claim"),Changed,*S->TvA.State,*S->TvB.State,S->TvB.AudioClock-S->TvA.AudioClock);
  }
  UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route waypoint %d/%d PASS at %.3f %.3f visibleFloor=%.4f normalWalkerFloor=%.4f"),S->Index+1,int32(UE_ARRAY_COUNT(Route)),P->Pos.X,P->Pos.Z,Floor,P->FloorY);
  ++S->Index;S->Since=Now;S->Reached=0;
 },.05f,true);
}
