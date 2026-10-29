// ART01: isolert kapittel-2-fotfixture, deretter vanlig Walker og HUD OnUse.
// Ingen bilreise, kinoåpning, menneskelig input eller visuell aksept hevdes.
#include "S47Web.h"
#include "S47Main.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "SS47Widgets.h"
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
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CinemaContract,Log,All);
namespace S47CinemaContract
{
 using FJ=S47J::FObj;
 const TCHAR* Closed=TEXT("Closed. The doors are chained. The posters behind the glass have faded to the same blue.");
 const TCHAR* PrintIds[]={TEXT("desert-sky"),TEXT("friday-matinee")};
 void Vec(const FJ& O,const TCHAR* Key,const FVector& V){S47J::Set(O,Key,TArray<double>{V.X,V.Y,V.Z});}
 bool Write(const FString& File,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*File);}
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 bool SameDoors(const TMap<FString,bool>& A,const TMap<FString,bool>& B)
 {
  if(A.Num()!=B.Num())return false;
  for(const auto& Pair:A){const bool* Found=B.Find(Pair.Key);if(!Found||*Found!=Pair.Value)return false;}return true;
 }
 double Angle(double V){return FMath::Atan2(FMath::Sin(V),FMath::Cos(V));}
 // The complete case except its advancing clock and notes. Flow adds an explicit
 // legacy-timeline note on Continue; all quest/chapter/board/native state remains compared.
 FJ Quest(const FJ& State)
 {
  const auto Original=S47J::Obj(State,TEXT("case"));if(!Original)return {};
  const auto Out=S47J::Clone(Original);Out->RemoveField(TEXT("clock"));Out->RemoveField(TEXT("notes"));return Out;
 }
 void SlateText(const TSharedRef<SWidget>& Widget,TArray<FString>& Out)
 {
  if(!Widget->GetVisibility().IsVisible())return;
  if(Widget->GetTypeAsString()==TEXT("STextBlock"))Out.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children=Widget->GetChildren();
  for(int32 I=0;Children&&I<Children->Num();++I)SlateText(Children->GetChildAt(I),Out);
 }
 bool Png(const FString& File,FJ& Receipt)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return false;
  const uint8 Signature[]={137,80,78,71,13,10,26,10};
  if(Bytes.Num()<33||FMemory::Memcmp(Bytes.GetData(),Signature,8))return false;
  auto U32=[&](int64 I){return (uint32(Bytes[I])<<24)|(uint32(Bytes[I+1])<<16)|(uint32(Bytes[I+2])<<8)|Bytes[I+3];};
  int64 At=8,Idat=0;int32 Chunks=0;uint32 Width=0,Height=0;bool End=false;
  while(At+12<=Bytes.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>Bytes.Num()-At-12)return false;
   const uint8* Type=Bytes.GetData()+At+4;if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(Chunks==0){if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13)return false;Width=U32(At+8);Height=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Idat+=Size;
   ++Chunks;At+=int64(Size)+12;if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=Bytes.Num()||Idat<=0||Width<320||Height<200)return false;
  FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);Receipt=S47J::New();
  S47J::Set(Receipt,TEXT("file"),File);S47J::Set(Receipt,TEXT("sha1"),Hash.ToString());S47J::Set(Receipt,TEXT("bytes"),Bytes.Num());
  S47J::Set(Receipt,TEXT("width"),double(Width));S47J::Set(Receipt,TEXT("height"),double(Height));S47J::Set(Receipt,TEXT("chunks"),Chunks);
  S47J::Set(Receipt,TEXT("all_chunk_crc_verified"),true);S47J::Set(Receipt,TEXT("iend_at_eof"),true);return true;
 }
 struct FOriginal
 {
  int32 Node=-1,Parent=-1,Geo=-1;TArray<int32> Mats;
  TWeakObjectPtr<USceneComponent> Component;TWeakObjectPtr<UStaticMesh> Mesh;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;FTransform Transform=FTransform::Identity;
  bool HiddenInGame=false;
  ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;
 };
 struct FControl:TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root,PhotoName,SaveId;
  bool Done=false,Started=false,Off=false,Requested=false,PhotoClosed=false;int32 Stage=0,Checks=0,Waypoint=0,ClosedUses=0,WalkSamples=0;
  double Began=FPlatformTime::Seconds(),Since=Began,Ready=0,Last=Began,ProgressAt=Began,PhotoAt=0,ContinueAt=0,WalkMetres=0;
  FVector LastFoot=FVector::ZeroVector,ProgressFoot=FVector::ZeroVector,Aim=FVector::ZeroVector,SavedFoot=FVector::ZeroVector;
  double SavedYaw=0,SavedPitch=0,SavedClock=0,SavedFloor=0,RestoredGt=0;
  TArray<FVector> Path;TArray<FOriginal> Originals;TArray<int32> PrintNodes;
  TMap<FString,bool> Doors;FJ InitialQuest,BeforeSave,PhotoMeta;
  TArray<TSharedPtr<FJsonValue>> Executed,Events,Snapshots,Photos,Rays,Route;
  US47Main* Main()const{return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  int32 Unique(const FString& Name)const
  {
   int32 Found=-1;for(int32 I=0;I<Web->PartCount();++I)if(const auto* Node=Web->Node(I))if(Node->Name==Name){if(Found>=0)return -1;Found=I;}return Found;
  }
  UStaticMeshComponent* Print(int32 Index)const
  {const auto* Node=PrintNodes.IsValidIndex(Index)?Web->Node(PrintNodes[Index]):nullptr;return Node?Cast<UStaticMeshComponent>(Node->Comp.Get()):nullptr;}
  FVector Poster(int32 Index)const
  {
   if(auto* Component=Print(Index))return S47Three::ToThree(Component->GetComponentLocation());
   // Off observes only the original frame's measured location, never claims new artwork.
   return S47Three::ToThree(Originals[0].Component->GetComponentTransform().TransformPosition(S47Three::ToUE(FVector(.162,1.51,Index==0?7:-7))));
  }
  FVector Proxy()const{return Web->Obj(TEXT("world.town.proxies.cinema")).WorldPosition();}
  void Look()const
  {
   auto* Player=Main()->Player.Get();const FVector D=Aim-S47Three::ToThree(Player->EyeUE());
   Player->Yaw=FMath::Atan2(-D.X,-D.Z);Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  void Result(bool Pass,const FString& Why)
  {
   const auto KeepAlive=AsShared();if(Done)return;Done=true;auto* M=Main();
   if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(Requested&&FScreenshotRequest::IsScreenshotRequested()&&FScreenshotRequest::GetFilename()==Root/(PhotoName+TEXT(".png")))FScreenshotRequest::Reset();
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("explicit_off"),Off);S47J::Set(O,TEXT("walk_metres"),WalkMetres);S47J::Set(O,TEXT("walk_samples"),WalkSamples);S47J::Set(O,TEXT("ordinary_closed_uses"),ClosedUses);
   S47J::Set(O,TEXT("pending_capture"),PhotoMeta);S47J::Set(O,TEXT("pending_capture_name"),PhotoName);
   if(M&&M->Player){Vec(O,TEXT("final_player_three_m"),M->Player->Pos);S47J::Set(O,TEXT("final_clock_seconds"),M->Story?M->Story->Clock():0.);}
   S47J::Set(O,TEXT("last_native_readback"),Web.IsValid()?Web->CinemaPostersReadback():FJ());
   O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("events"),Events);O->SetArrayField(TEXT("snapshots"),Snapshots);
   O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("interaction_rays"),Rays);O->SetArrayField(TEXT("physical_route_samples"),Route);
   S47J::Set(O,TEXT("scope"),TEXT("Isolated chapter2/case0 and one declared legal exterior foot start; continuous ordinary Walker, actual camera ray/focus and Player.OnUse; explicit World.Enter visibility roundtrip; entire actual case0 snapshot copied unchanged to isolated case1, ordinary title/Continue, fresh disk readers and normal post-Continue save. No car or commute claim."));
   S47J::Set(O,TEXT("limits"),Off?TEXT("Explicit Off: zero new prints, original/proxy/Closed and disk journey only; new artwork audit NOT CLAIMED. API input, not human/visual/FPS acceptance. No cinema interior, quest completion, full matrix, installation or release."):TEXT("API input, not human or visual/FPS acceptance. Static day/dusk/night suites are separate. No cinema interior, new quest, full matrix, installation or release."));
   const bool Written=Write(Root/TEXT("result.json"),O);if(Web.IsValid())Web->GetWorldTimerManager().ClearTimer(Timer);
   if(Pass&&Written){UE_LOG(LogS47CinemaContract,Display,TEXT("S47: CinemaPosters contract PASS checks=%d walk=%.3fm evidence=%s"),Checks,WalkMetres,*Root);FPlatformMisc::RequestExitWithStatus(false,0);}
   else{UE_LOG(LogS47CinemaContract,Error,TEXT("S47: CinemaPosters contract FAIL stage=%d: %s evidence=%s"),Stage,Written?*Why:TEXT("final result write failed"),*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Good,const FString& Why)
  {
   ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("detail"),Why);S47J::Set(O,TEXT("at_seconds"),FPlatformTime::Seconds()-Began);Executed.Add(MakeShared<FJsonValueObject>(O));
   if(!Good){Result(false,Why);return false;}return true;
  }
  void Next(int32 Value,const FString& Why)
  {
   Stage=Value;Since=FPlatformTime::Seconds();auto O=S47J::New();S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("detail"),Why);S47J::Set(O,TEXT("at_seconds"),Since-Began);Events.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogS47CinemaContract,Display,TEXT("S47 CinemaPosters stage %d: %s"),Stage,*Why);
  }
  bool CaptureOriginals()
  {
   const int32 Nodes[]={Unique(TEXT("town-production:SM_Town_Cinema_Shell")),Unique(TEXT("town-production:SM_Town_Cinema_Glass")),1514,1515};
   for(int32 Index=0;Index<4;++Index)
   {
    const auto* Node=Web->Node(Nodes[Index]);auto* Component=Node?Node->Comp.Get():nullptr;
    if(!Check(Node&&Component,TEXT("complete original Shell/Glass/proxy/hit leaf before fixture")))return false;
    FOriginal Saved;Saved.Node=Nodes[Index];Saved.Parent=Node->Parent;Saved.Geo=Node->Geo;Saved.Mats=Node->Mats;Saved.Component=Component;Saved.Transform=Component->GetComponentTransform();Saved.HiddenInGame=Component->bHiddenInGame;
    if(auto* Static=Cast<UStaticMeshComponent>(Component))
    {
     Saved.Mesh=Static->GetStaticMesh().Get();Saved.Collision=Static->GetCollisionEnabled();for(int32 Slot=0;Slot<Static->GetNumMaterials();++Slot)Saved.Materials.Add(Static->GetMaterial(Slot));
    }
    if(Index<2)
    {
     const FString Name=Index==0?TEXT("SM_Town_Cinema_Shell"):TEXT("SM_Town_Cinema_Glass");
     if(!Check(Saved.Mesh.IsValid()&&Saved.Mesh->GetPathName()==TEXT("/Game/TownBuildings/")+Name+TEXT(".")+Name&&Saved.Mesh->GetNumLODs()==3&&Node->Parent==Web->Handle(TEXT("world.town.group")),TEXT("actual three-LOD TB2 cinema predecessor and parent")))return false;
    }
    Originals.Add(Saved);
   }
   if(!Check(Web->Handle(TEXT("world.town.proxies.cinema"))==1514&&Web->Node(1515)->Parent==1514&&Web->Node(1515)->Geo==1050,TEXT("exact original cinema proxy1514 and g1050 hit leaf1515")))return false;
   for(int32 I=0;I<2;++I)PrintNodes.Add(Unique(FString(TEXT("cinema-poster:"))+PrintIds[I]));
   return true;
  }
  bool Snapshot(const FString& Name,bool Visible)
  {
   auto* M=Main();auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("area"),M->World->Area);S47J::Set(O,TEXT("frame"),double(GFrameCounter));
   S47J::Set(O,TEXT("clock_seconds"),M->Story->Clock());Vec(O,TEXT("player_three_m"),M->Player->Pos);
   const auto Readback=Web->CinemaPostersReadback();S47J::Set(O,TEXT("native_readback"),Readback);
   TArray<FString> Lines;const bool Audit=Web->AuditCinemaPosters(Lines);S47J::Set(O,TEXT("art_audit_pass"),Audit);S47J::Set(O,TEXT("art_audit_lines"),Lines);S47J::Set(O,TEXT("new_art_audit_claimed"),!Off);
   TArray<TSharedPtr<FJsonValue>> ProtectedRows;bool Intact=Originals.Num()==4;
   for(const auto& Saved:Originals)
   {
    const auto* Node=Web->Node(Saved.Node);auto* Component=Node?Node->Comp.Get():nullptr;auto Row=S47J::New();
    bool Good=Node&&Component&&Node->Comp==Saved.Component&&Node->Parent==Saved.Parent&&Node->Geo==Saved.Geo&&Node->Mats==Saved.Mats&&Component->GetComponentTransform().Equals(Saved.Transform,.00001)&&Component->bHiddenInGame==Saved.HiddenInGame;
    S47J::Set(Row,TEXT("node"),Saved.Node);S47J::Set(Row,TEXT("component"),GetPathNameSafe(Component));S47J::Set(Row,TEXT("metadata_shown"),Node&&Web->ObjAt(Saved.Node).Shown());
    S47J::Set(Row,TEXT("actual_component_object_id"),double(Component?Component->GetUniqueID():0));S47J::Set(Row,TEXT("baseline_component_object_id"),double(Saved.Component.IsValid()?Saved.Component->GetUniqueID():0));
    S47J::Set(Row,TEXT("hidden_in_game_baseline"),Saved.HiddenInGame);S47J::Set(Row,TEXT("actual_hidden_in_game"),Component&&Component->bHiddenInGame);
    if(auto* Static=Cast<UStaticMeshComponent>(Component))
    {
     Good&=Static->GetStaticMesh()==Saved.Mesh.Get()&&Static->GetNumMaterials()==Saved.Materials.Num()&&Static->GetCollisionEnabled()==Saved.Collision;
     TArray<FString> MaterialPaths;for(int32 Slot=0;Slot<Static->GetNumMaterials();++Slot){Good&=Saved.Materials.IsValidIndex(Slot)&&Static->GetMaterial(Slot)==Saved.Materials[Slot].Get();MaterialPaths.Add(GetPathNameSafe(Static->GetMaterial(Slot)));}
     S47J::Set(Row,TEXT("actual_materials"),MaterialPaths);S47J::Set(Row,TEXT("actual_mesh"),GetPathNameSafe(Static->GetStaticMesh().Get()));
     if(Saved.Node==Originals[0].Node||Saved.Node==Originals[1].Node)Good&=Static->IsVisible()==Visible&&Web->ObjAt(Saved.Node).Shown()==Visible;
    }
    if(Saved.Node==1514)Good&=Web->ObjAt(1514).Shown()==Visible;
    S47J::Set(Row,TEXT("bindings_unchanged"),Good);ProtectedRows.Add(MakeShared<FJsonValueObject>(Row));Intact&=Good;
   }
   O->SetArrayField(TEXT("original_bindings"),ProtectedRows);
   const auto* Interaction=M->Inter->Get(TEXT("town:cinema"));
   const bool ProxyIntact=Interaction&&Interaction->Object.W==Web.Get()&&Interaction->Object.I==1514&&FMath::Abs(Interaction->Range-2.6)<1e-9&&!Interaction->bLook&&bool(Interaction->Use)&&bool(Interaction->Label)&&bool(Interaction->Examine);
   S47J::Set(O,TEXT("actual_interaction_range_m"),Interaction?Interaction->Range:-1.);S47J::Set(O,TEXT("actual_interaction_label"),M->Inter->LabelOf(TEXT("town:cinema")));
   int32 Count=0;for(int32 I=0;I<Web->PartCount();++I)if(const auto* Node=Web->Node(I))Count+=Node->Name.StartsWith(TEXT("cinema-poster:"));
   S47J::Set(O,TEXT("actual_added_print_count"),Count);Snapshots.Add(MakeShared<FJsonValueObject>(O));
   if(!Write(Root/(Name+TEXT(".json")),O)){Result(false,TEXT("scene snapshot write failed"));return false;}
   if(!Check(Intact&&ProxyIntact,TEXT("same original Shell/Glass/all materials/collision/proxy1514/range2.6 across lifecycle: ")+Name))return false;
   if(Off)return Check(!Audit&&Count==0&&(!Readback||!S47J::Bool(Readback,TEXT("active"))),TEXT("explicit Off: zero additions, original journey only, no new-art acceptance: ")+Name);
   if(!Check(Audit&&Count==2&&Readback&&S47J::Bool(Readback,TEXT("active"))&&S47J::Bool(Readback,TEXT("originals_intact"))&&S47J::Bool(Readback,TEXT("expected_visible"))==Visible&&S47J::Arr(Readback,TEXT("prints")).Num()==2,TEXT("both actual material/texture/UV/native bindings pass adapter audit: ")+Name))return false;
   for(int32 I=0;I<2;++I)if(!Check(Print(I)&&Web->Node(PrintNodes[I])->Name==FString(TEXT("cinema-poster:"))+PrintIds[I]&&Print(I)->IsVisible()==Visible&&Print(I)->bHiddenInGame==!Visible&&Web->ObjAt(PrintNodes[I]).Shown()==Visible,TEXT("same unique native poster component follows actual area visibility")))return false;
   return true;
  }
  bool Floor()
  {
   auto* Player=Main()->Player.Get();FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(S47CinemaFloor),true,Player);
   const bool HitGround=Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Player->Pos.X,Player->FloorY+.12,Player->Pos.Z),S47Three::ToUE(Player->Pos.X,Player->FloorY-.35,Player->Pos.Z),ECC_Visibility,Query);
   return Check(Player->Walkable(Player->Pos.X,Player->Pos.Z)&&Player->Pos.X-Player->Radius>7985.25&&HitGround&&Hit.ImpactNormal.Z>.7&&FMath::Abs(S47Three::ToThree(Hit.ImpactPoint).Y-Player->FloorY)<.08,TEXT("ordinary Walker and rendered ground remain outside the original closed cinema"));
  }
  void WalkTo(const TArray<FVector>& Points)
  {Path=Points;Waypoint=0;LastFoot=ProgressFoot=Main()->Player->Pos;Last=ProgressAt=FPlatformTime::Seconds();}
  bool Walk(double Now)
  {
   auto* Player=Main()->Player.Get();const double Moved=FVector2D(Player->Pos.X-LastFoot.X,Player->Pos.Z-LastFoot.Z).Size();
   if(Moved>FMath::Max(.5,(Now-Last)*4+.1)){Result(false,TEXT("nonphysical Walker discontinuity; no placement recovery"));return false;}
   WalkMetres+=Moved;LastFoot=Player->Pos;Last=Now;++WalkSamples;
   auto Sample=S47J::New();Vec(Sample,TEXT("player_three_m"),Player->Pos);S47J::Set(Sample,TEXT("walk_metres"),WalkMetres);S47J::Set(Sample,TEXT("at_seconds"),Now-Began);Route.Add(MakeShared<FJsonValueObject>(Sample));
   if(Player->Pos.X-Player->Radius<=7985.25){Result(false,TEXT("physical route crossed the existing facade/glass plane"));return false;}
   if(FVector2D(Player->Pos.X-ProgressFoot.X,Player->Pos.Z-ProgressFoot.Z).Size()>.15){ProgressFoot=Player->Pos;ProgressAt=Now;}
   if(Waypoint>=Path.Num()){Player->TestMove.Reset();return true;}
   const FVector Delta=Path[Waypoint]-Player->Pos;const double Distance=FVector2D(Delta.X,Delta.Z).Size();
   if(Distance<.085){Player->TestMove.Reset();if(!Floor())return false;++Waypoint;return false;}
   if(Now-ProgressAt>12){Result(false,TEXT("ordinary exterior Walker stalled; no collision or tolerance fallback"));return false;}
   Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);Player->Pitch=-.08;Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.5));Player->bTestRun=false;return false;
  }
  bool ClosedSlate(TArray<FString>& Text)const
  {
   auto* M=Main();if(M->Hud&&M->Hud->HudWidget())SlateText(M->Hud->HudWidget()->AsShared(),Text);
   return Text.Contains(FString(Closed))&&M->UI->LastDescription()==Closed&&!M->UI->Modal();
  }
  bool UseClosed()
  {
   auto* M=Main();auto* Player=M->Player.Get();const auto* Interaction=M->Inter->Get(TEXT("town:cinema"));
   const FVector Eye=Player->EyeUE(),Forward=Player->ForwardUE();const FString Pick=M->Inter->Update(Eye,Forward);
   auto Ray=S47J::New();Vec(Ray,TEXT("eye_ue_cm"),Eye);Vec(Ray,TEXT("forward_ue"),Forward);S47J::Set(Ray,TEXT("picked"),Pick);S47J::Set(Ray,TEXT("focused"),Player->FocusedId());S47J::Set(Ray,TEXT("walk_metres"),WalkMetres);
   TArray<FHitResult> Hits;FCollisionQueryParams Query(SCENE_QUERY_STAT(S47CinemaUse),true);
   Web->GetWorld()->LineTraceMultiByChannel(Hits,Eye,Eye+Forward.GetSafeNormal()*600,ECC_GameTraceChannel1,Query);
   bool ActualHit=false;for(const auto& Hit:Hits)if(!Web->UsableAt(Hit.GetComponent()).IsEmpty())
   {
    S47J::Set(Ray,TEXT("first_usable_component"),GetPathNameSafe(Hit.GetComponent()));S47J::Set(Ray,TEXT("first_usable_node"),Web->NodeOf(Hit.GetComponent()));S47J::Set(Ray,TEXT("hit_distance_m"),Hit.Distance/100.);S47J::Set(Ray,TEXT("actual_range_m"),Interaction?Interaction->Range:-1.);
    ActualHit=Interaction&&Web->UsableAt(Hit.GetComponent())==TEXT("town:cinema")&&Web->NodeOf(Hit.GetComponent())==1515&&Hit.Distance>0&&Hit.Distance/100.<=Interaction->Range;break;
   }
   Rays.Add(MakeShared<FJsonValueObject>(Ray));
   if(!Check(ActualHit&&Pick==TEXT("town:cinema")&&Player->FocusedId()==Pick&&bool(Player->OnUse)&&!M->UI->Modal()&&M->Inter->LabelOf(Pick)==TEXT("The Plaza"),TEXT("actual settled player camera/focus hits original cinema leaf inside actual range2.6 before normal HUD OnUse")))return false;
   M->UI->ClearToasts();Player->OnUse(Pick);++ClosedUses;return true;
  }
  void QueuePhoto(const FString& Name,const FVector& Target,bool NeedClosed=false)
  {PhotoName=Name;Aim=Target;PhotoClosed=NeedClosed;Requested=false;PhotoAt=FPlatformTime::Seconds();PhotoMeta.Reset();Look();}
  bool Photo(int32 PosterIndex=-1)
  {
   Look();const double Now=FPlatformTime::Seconds();const FString File=Root/(PhotoName+TEXT(".png"));auto* M=Main();
   if(!Requested)
   {
    if(Now-PhotoAt<.6)return false;
    if(!Check(!FScreenshotRequest::IsScreenshotRequested()&&!IFileManager::Get().FileExists(*File),TEXT("own fresh viewport capture, no competing screenshot request")))return false;
    PhotoMeta=S47J::New();S47J::Set(PhotoMeta,TEXT("name"),PhotoName);Vec(PhotoMeta,TEXT("player_three_m"),M->Player->Pos);Vec(PhotoMeta,TEXT("eye_ue_cm"),M->Player->EyeUE());Vec(PhotoMeta,TEXT("forward_ue"),M->Player->ForwardUE());Vec(PhotoMeta,TEXT("target_three_m"),Aim);S47J::Set(PhotoMeta,TEXT("frame"),double(GFrameCounter));
    auto* Controller=Cast<APlayerController>(M->Player->GetController());const auto* Camera=Controller?Controller->PlayerCameraManager.Get():nullptr;
    if(!Check(Controller&&Camera,TEXT("actual final camera manager is available")))return false;
    Vec(PhotoMeta,TEXT("camera_manager_ue_cm"),Camera->GetCameraLocation());S47J::Set(PhotoMeta,TEXT("fov_degrees"),double(Camera->GetFOVAngle()));
    if(PosterIndex>=0)
    {
     FVector2D Size=FVector2D::ZeroVector;if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->GetViewportSize(Size);
     double LowX=1e20,LowY=1e20,HighX=-1e20,HighY=-1e20;bool Inside=Size.X>=320&&Size.Y>=200;
     for(int32 A:{-1,1})for(int32 B:{-1,1})
     {
      const auto* Component=Print(PosterIndex);const FVector Corner=Component?Component->GetComponentTransform().TransformPosition(FVector(50*A,50*B,0)):S47Three::ToUE(Poster(PosterIndex)+FVector(0,.805*B,.565*A));
      FVector2D Pixel=FVector2D::ZeroVector;Inside&=Controller->ProjectWorldLocationToScreen(Corner,Pixel,true);LowX=FMath::Min(LowX,Pixel.X);LowY=FMath::Min(LowY,Pixel.Y);HighX=FMath::Max(HighX,Pixel.X);HighY=FMath::Max(HighY,Pixel.Y);
      Inside&=Pixel.X>=8&&Pixel.Y>=8&&Pixel.X<=Size.X-8&&Pixel.Y<=Size.Y-8;
     }
     S47J::Set(PhotoMeta,TEXT("projected_bounds_px"),TArray<double>{LowX,LowY,HighX,HighY});S47J::Set(PhotoMeta,TEXT("viewport_px"),TArray<double>{Size.X,Size.Y});S47J::Set(PhotoMeta,TEXT("new_art_audit_claimed"),!Off);
     if(!Check(Inside&&HighX-LowX>=80&&HighY-LowY>=120&&S47Three::ToThree(M->Player->EyeUE()).X>Aim.X+.1,TEXT("entire actual poster/frame projects from its legal exterior front at meaningful pixel size")))return false;
    }
    if(PhotoClosed){TArray<FString> Text;if(!Check(ClosedSlate(Text),TEXT("original Closed/doors-chained toast actually exists in visible Slate at capture request")))return false;S47J::Set(PhotoMeta,TEXT("visible_slate_text"),Text);}
    FScreenshotRequest::RequestScreenshot(File,true,false);Requested=true;return false;
   }
   FJ Receipt;if(!Png(File,Receipt)){if(Now-PhotoAt>15)Result(false,TEXT("complete viewport PNG/CRC/IEND missing: ")+PhotoName);return false;}
   S47J::Set(PhotoMeta,TEXT("png"),Receipt);Photos.Add(MakeShared<FJsonValueObject>(PhotoMeta));
   if(!Write(Root/(PhotoName+TEXT(".capture.json")),PhotoMeta)){Result(false,TEXT("capture receipt write failed"));return false;}Requested=false;return true;
  }
  bool DialogSame(const FJ& A,const FJ& B)
  {
   const auto Before=S47J::Obj(A,TEXT("evening")),After=S47J::Obj(B,TEXT("evening"));
   for(const TCHAR* Key:{TEXT("pack"),TEXT("radioPack"),TEXT("takeaway")})if(!Same(S47J::Obj(Before,Key),S47J::Obj(After,Key)))return false;return true;
  }
  void Tick()
  {
   const auto KeepAlive=AsShared();if(Done)return;const double Now=FPlatformTime::Seconds();auto* M=Main();
   if(Now-Began>180){Result(false,TEXT("180 second targeted contract timeout; later unexecuted stages are not accepted"));return;}
   if(!M||!M->Flow||!M->Story||!M->World||!M->Player||!M->Inter||!M->UI||!M->Evening||!M->TownLayer||!M->TownLayer->Caliche||S47Startup::Blocking()){Ready=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){Ready=0;return;}if(Ready==0){Ready=Now;return;}if(Now-Ready<2)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Flow->Saves.List().IsEmpty()&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()))&&M->Story->ChapterKey==TEXT("ch2")&&!M->Story->bEveningTimeline&&!M->Evening->Opening()&&!M->World->bDriving&&!M->UI->Modal()&&!M->Story->IsCinematic(),TEXT("fresh isolated chapter2/case0, real profile-local save store, no saved case/commute/modal/cinematic before sole foot fixture")))return;
    if(!CaptureOriginals())return;InitialQuest=Quest(M->Flow->GameState());if(!Check(InitialQuest.IsValid(),TEXT("actual complete chapter2 case exists")))return;
    auto Fixture=S47J::New();S47J::Set(Fixture,TEXT("kind"),TEXT("EXPLICIT legal exterior FOOT START; no car, egress, original spawn or prior traversal acceptance"));Vec(Fixture,TEXT("foot_three_x_y_z"),FVector(7991,0,2175));S47J::Set(Fixture,TEXT("clock_before"),M->Story->Clock());
    if(!Write(Root/TEXT("explicit-foot-fixture.json"),Fixture)){Result(false,TEXT("foot fixture provenance write failed"));return;}
    Started=true;M->World->Enter(TEXT("diner"));M->Player->Place(7991,2175,0);Next(1,TEXT("sole explicit exterior foot start; all subsequent approaches use ordinary Walker; native clock unchanged"));return;
   }
   if(M->Flow->bStarting||M->World->bBusy)return;
   if(Stage!=11&&M->Mode!=US47Main::EMode::Play){Result(false,TEXT("unexpected non-play mode before ordinary title/Continue stage"));return;}
   if(Stage==1)
   {
    if(Now-Since<.8)return;if(!Floor()||!Snapshot(TEXT("arrival"),true))return;
    WalkTo({FVector(7988,0,2175),FVector(7988,0,2169)});Next(2,TEXT("physical legal exterior approach to desert-sky frame"));return;
   }
   if(Stage==2){if(!Walk(Now))return;QueuePhoto(TEXT("desert-sky"),Poster(0));Next(3,TEXT("settle actual walking camera on first poster"));return;}
   if(Stage==3)
   {
    if(!Photo(0)||!Snapshot(TEXT("first-poster"),true))return;WalkTo({FVector(7988,0,2155)});Next(4,TEXT("continuous Walker along closed facade to friday-matinee frame"));return;
   }
   if(Stage==4){if(!Walk(Now))return;QueuePhoto(TEXT("friday-matinee"),Poster(1));Next(5,TEXT("settle actual camera on second poster"));return;}
   if(Stage==5)
   {
    if(!Photo(1)||!Snapshot(TEXT("second-poster"),true))return;WalkTo({FVector(7988,0,2162),FVector(7987.5,0,2162)});Next(6,TEXT("physical return to the unchanged closed entrance proxy, still outside glass"));return;
   }
   if(Stage==6){if(!Walk(Now))return;Aim=Proxy();Look();Next(7,TEXT("settle actual ray/focus before ordinary Player.OnUse"));return;}
   if(Stage==7)
   {
    Look();if(Now-Since<.35)return;if(!UseClosed())return;QueuePhoto(TEXT("plaza-closed"),Proxy(),true);Next(8,TEXT("actual original Closed response, no interaction bypass or new quest"));return;
   }
   if(Stage==8)
   {
    if(!Photo()||!Snapshot(TEXT("original-closed"),true))return;if(!Check(Same(InitialQuest,Quest(M->Flow->GameState())),TEXT("poster approach and normal Closed interaction changed no chapter/quest/board state")))return;
    M->Player->TestMove.Reset();M->World->Enter(TEXT("saro"));Next(9,TEXT("EXPLICIT World.Enter visibility control, not a physically traversed area journey"));return;
   }
   if(Stage==9)
   {
    if(Now-Since<.35)return;if(!Check(M->World->Area==TEXT("saro"),TEXT("actual area changed to SARO"))||!Snapshot(TEXT("area-hidden"),false))return;
    M->World->Enter(TEXT("diner"));Next(10,TEXT("actual World.Enter return; original and new components must recover normally"));return;
   }
   if(Stage==10)
   {
    Aim=Proxy();Look();if(Now-Since<.8)return;if(!Floor()||!Snapshot(TEXT("area-return"),true))return;
    if(!Check(WalkMetres>25&&WalkSamples>100&&M->Flow->GameSaveBlock().IsEmpty()&&!M->UI->Modal(),TEXT("both physical poster approaches and ordinary closed entrance completed at legal gameplay save boundary")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedFloor=M->Player->FloorY;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();Doors=Web->DoorStates();
    if(!Check(S47J::Str(BeforeSave,TEXT("checkpoint"))==TEXT("chapter2")&&Same(InitialQuest,Quest(BeforeSave)),TEXT("real chapter2 checkpoint retained, no quest fabrication before full disk fixture")))return;
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=M->Story->ChapterTitle();Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;FString Error;
    const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT DISK FIXTURE: unchanged full actual case0 snapshot saved as isolated case1: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),BeforeSave),TEXT("fresh independent reader equals complete actual pre-title disk snapshot")))return;
    if(!Write(Root/TEXT("before-continue.disk.json"),BeforeSave)){Result(false,TEXT("pre-title disk evidence write failed"));return;}
    SaveId=Meta->Id;M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(11,TEXT("ordinary title then exact isolated full Flow Continue"));return;
   }
   if(Stage==11)
   {
    if(Now-Since<.6)return;const auto Meta=M->Flow->Saves.ContinueSave();
    if(!Check(M->Mode==US47Main::EMode::Title&&Meta&&Meta->Id==SaveId,TEXT("ordinary title resolves the exact actual disk save"))||!Snapshot(TEXT("title-hidden"),false))return;
    ContinueAt=Now;if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary Continue action accepted")))return;
    // OpenTestView -> OpenSave -> StartGame is synchronous. Measure the restored
    // native clock before another game frame, then compare its real game-time advance.
    RestoredGt=M->Story->Gt();
    if(!Check(M->Mode==US47Main::EMode::Play&&!M->Flow->bStarting&&FMath::Abs(M->Story->Clock()-SavedClock)<.001,TEXT("exact saved native clock immediately after full synchronous Continue, before any new frame")))return;
    Next(12,TEXT("full OpenSave/StartGame lifecycle, not direct subsystem restore"));return;
   }
   if(Stage==12)
   {
    if(Now-Since<1||!M->Flow->SaveBlock().IsEmpty())return;const auto Actual=M->Flow->GameState();
    const double ClockDelta=M->Story->Clock()-SavedClock,GameDelta=M->Story->Gt()-RestoredGt;
    bool Restored=M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->Story->ChapterKey==TEXT("ch2")&&M->World->Area==TEXT("diner")&&!M->World->bDriving&&M->Player->Pos.Equals(SavedFoot,.015)
     &&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Player->FloorY-SavedFloor)<.08
     &&ClockDelta>=0&&FMath::Abs(ClockDelta-GameDelta)<.025&&SameDoors(Doors,Web->DoorStates())&&Same(Quest(BeforeSave),Quest(Actual))&&DialogSame(BeforeSave,Actual)&&!M->UI->Modal();
    for(const auto& Key:S47J::Strs(BeforeSave,TEXT("town")))Restored&=M->TownSeen.Contains(Key);
    auto Receipt=S47J::New();S47J::Set(Receipt,TEXT("expected"),BeforeSave);S47J::Set(Receipt,TEXT("actual"),Actual);S47J::Set(Receipt,TEXT("clock_delta_seconds"),ClockDelta);S47J::Set(Receipt,TEXT("actual_native_game_time_since_continue_seconds"),GameDelta);S47J::Set(Receipt,TEXT("wall_seconds_since_continue"),Now-ContinueAt);S47J::Set(Receipt,TEXT("pass"),Restored);
    if(!Write(Root/TEXT("continue-readback.json"),Receipt)){Result(false,TEXT("Continue readback write failed"));return;}
    if(!Check(Restored,TEXT("ordinary chapter2 Continue retained actual area/player/view/native floor/clock/doors/whole quest case and saved dialogue packs, without clock rewriting"))||!Snapshot(TEXT("after-continue"),true))return;
    Aim=Proxy();Look();Next(13,TEXT("fresh restored camera must still reach original Closed via ordinary HUD OnUse"));return;
   }
   if(Stage==13){Look();if(Now-Since<.35)return;if(!UseClosed())return;Next(14,TEXT("same original Closed action after full Continue"));return;}
   if(Stage==14)
   {
    if(Now-Since<.2)return;TArray<FString> Text;if(!Check(ClosedSlate(Text),TEXT("ordinary post-Continue OnUse still opens original visible Closed response")))return;
    WalkTo({FVector(7988,0,2162),FVector(7988,0,2169)});Next(15,TEXT("physical restored Walker returns to first poster for final viewport proof"));return;
   }
   if(Stage==15){if(!Walk(Now))return;QueuePhoto(TEXT("after-continue-poster"),Poster(0));Next(16,TEXT("actual post-Continue walking camera and native art"));return;}
   if(Stage==16)
   {
    if(!Photo(0)||!Snapshot(TEXT("final-post-continue-poster"),true))return;
    const auto Expected=M->Flow->GameState();FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary case1 Flow.WriteSave after physical restored journey: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Actual=Reader.Load(Meta->Id);if(!Check(Same(Expected,Actual),TEXT("fresh independent reader equals entire ordinary post-Continue save")))return;
    if(!Write(Root/TEXT("after-continue-normal-save.disk.json"),Actual)){Result(false,TEXT("normal post-Continue disk evidence write failed"));return;}Next(17,TEXT("all full PNG receipts and required executed stages before PASS"));return;
   }
   if(Stage==17)
   {
    if(!Check(Photos.Num()==4&&ClosedUses==2&&WalkMetres>30,TEXT("both poster views, original Closed UI, after-Continue view and both ordinary OnUse actions actually completed")))return;
    for(const auto& Value:Photos){const auto Old=S47J::Obj(Value->AsObject(),TEXT("png"));FJ Fresh;if(!Check(Png(S47J::Str(Old,TEXT("file")),Fresh)&&Same(Old,Fresh),TEXT("final unchanged complete viewport PNG signature/chunk CRC/IEND/hash receipt")))return;}
    Result(true,Off?TEXT("explicit Off preserves original closed Plaza/proxy and complete physical/disk journey; no new-art audit claimed"):TEXT("two native exterior posters, legal physical approach, original Closed HUD interaction, area lifecycle and full chapter2 disk/title/Continue"));
   }
  }
 };
}

void AS47Web::StartCinemaPostersContract()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47CinemaPostersContract")))return;
 FString UserDir,Jump,Value;const bool HasDir=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 const bool RawSafe=HasDir&&!UserDir.Contains(TEXT(".."));UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 const bool Safe=RawSafe&&(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir);
 if(!Safe){UE_LOG(LogS47CinemaContract,Error,TEXT("S47: CinemaPosters contract FAIL requires explicit isolated /tmp UserDir; no personal Saved may be used"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto State=MakeShared<S47CinemaContract::FControl>();State->Web=this;State->Off=FParse::Param(FCommandLine::Get(),TEXT("S47CinemaPostersOff"));
 const FString Base=FPaths::ProjectSavedDir()/TEXT("CinemaPostersContract");const bool Fresh=!IFileManager::Get().DirectoryExists(*Base);
 State->Root=Base/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*State->Root,true)){UE_LOG(LogS47CinemaContract,Error,TEXT("S47: CinemaPosters contract FAIL evidence directory"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 bool Conflict=false;
 for(const TCHAR* Key:{TEXT("S47Case="),TEXT("S47Evening="),TEXT("S47EveningRoute="),TEXT("S47Drive="),TEXT("S47Walk="),TEXT("S47WalkFrom="),TEXT("S47Face="),TEXT("S47Auto="),TEXT("S47Hide="),TEXT("S47ProductionSuite="),TEXT("S47Open="),TEXT("S47HorrorContract=")})Conflict|=FParse::Value(FCommandLine::Get(),Key,Value);
 for(const TCHAR* Flag:{TEXT("NullRHI"),TEXT("S47Title"),TEXT("S47Evening"),TEXT("S47SmokeTest"),TEXT("S47ProductionViews"),TEXT("S47ProductionCab"),TEXT("S47FarmAccessContract"),TEXT("S47CalicheHomesContract"),TEXT("S47CalicheHomesContractFallback"),TEXT("S47RadioWomanContract"),TEXT("S47RadioWomanConversation"),TEXT("S47DaleDocumentsContract"),TEXT("S47MotelV2Route"),TEXT("S47NightSequenceContract"),TEXT("S47T01DriveContract"),TEXT("S47T01Contract"),TEXT("S47T02Contract"),TEXT("S47T02DriveContract"),TEXT("S47HorrorArrival"),TEXT("S47HorrorContract"),TEXT("S47GetOut"),TEXT("S47EveningEntrance"),TEXT("S47TownBuildingsOff"),TEXT("S47PropsOff"),TEXT("S47LegacyCalicheStreet"),TEXT("S47EveningTimeline")})Conflict|=FParse::Param(FCommandLine::Get(),Flag);
 if(!State->Check(Fresh&&!Conflict&&FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump)&&Jump==TEXT("chapter2"),TEXT("fresh empty contract profile, -S47Jump=chapter2, normal assets/RHI and no competing pose/route/clock driver")))return;
 GetWorldTimerManager().SetTimer(State->Timer,[State]{State->Tick();},.05f,true);
 UE_LOG(LogS47CinemaContract,Display,TEXT("S47: CinemaPosters contract armed off=%d; explicit chapter2 foot fixture, ordinary Walker/HUD OnUse and disk Continue; no car/human/visual claim; evidence=%s"),State->Off,*State->Root);
}
