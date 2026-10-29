// HUM01: egen kapittel-2-fot-/18:30-fixture og ordinær samtaleinngang.
// Ingen bil, LOD-, såle- eller menneskelig aksept hevdes i denne kontrakten.
#include "S47Web.h"
#include "S47Main.h"
#include "S47Caliche.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "SS47Widgets.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogS47RadioWomanConversation,Log,All);
namespace S47RadioWomanConversation
{
 using FJ=S47J::FObj;
 bool Write(const FString& Path,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*Path);}
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 void Vec(const FJ& O,const TCHAR* Key,const FVector& V){S47J::Set(O,Key,TArray<double>{V.X,V.Y,V.Z});}
 double Angle(double V){return FMath::Atan2(FMath::Sin(V),FMath::Cos(V));}
 bool SameDoors(const TMap<FString,bool>& A,const TMap<FString,bool>& B)
 {if(A.Num()!=B.Num())return false;for(const auto& Pair:A){const bool* Found=B.Find(Pair.Key);if(!Found||*Found!=Pair.Value)return false;}return true;}
 FJ Quest(const FJ& State)
 {
  const auto Original=S47J::Obj(State,TEXT("case"));if(!Original)return {};
  const auto Result=S47J::Clone(Original);Result->RemoveField(TEXT("clock"));Result->RemoveField(TEXT("notes"));return Result;
 }
 void TextOf(const TSharedRef<SWidget>& Widget,TArray<FString>& Out)
 {
  if(!Widget->GetVisibility().IsVisible())return;
  if(Widget->GetTypeAsString()==TEXT("STextBlock"))Out.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children=Widget->GetChildren();for(int32 I=0;Children&&I<Children->Num();++I)TextOf(Children->GetChildAt(I),Out);
 }
 bool Png(const FString& Path,FJ& Out)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*Path))return false;const uint8 Signature[]={137,80,78,71,13,10,26,10};
  if(B.Num()<33||FMemory::Memcmp(B.GetData(),Signature,8))return false;
  auto U32=[&](int64 I){return (uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;uint32 Width=0,Height=0;int32 Chunks=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>B.Num()-At-12)return false;const uint8* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(Chunks==0){if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13)return false;Width=U32(At+8);Height=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Idat+=Size;++Chunks;At+=int64(Size)+12;if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=B.Num()||Idat<=0||Width<320||Height<200)return false;FSHAHash Hash;FSHA1::HashBuffer(B.GetData(),B.Num(),Hash.Hash);
  Out=S47J::New();S47J::Set(Out,TEXT("file"),Path);S47J::Set(Out,TEXT("sha1"),Hash.ToString());S47J::Set(Out,TEXT("bytes"),B.Num());S47J::Set(Out,TEXT("width"),double(Width));S47J::Set(Out,TEXT("height"),double(Height));S47J::Set(Out,TEXT("chunks"),Chunks);S47J::Set(Out,TEXT("all_chunk_crc_verified"),true);S47J::Set(Out,TEXT("iend_at_eof"),true);return true;
 }
 struct FBinding
 {
  int32 Node=-1,Parent=-1,Geo=-1;TArray<int32> Mats;TWeakObjectPtr<USceneComponent> Component;TWeakObjectPtr<UStaticMesh> Mesh;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;FTransform Transform=FTransform::Identity;ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;bool Hidden=false;
 };
 struct FControl:TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root,PhotoName,SaveId;TWeakPtr<SWidget> OwnedModal;
  bool Done=false,Started=false,Requested=false;int32 Stage=0,Checks=0,NativeNode=-1,Waypoint=0,WalkSamples=0,Uses=0;
  double Began=FPlatformTime::Seconds(),Since=Began,Ready=0,Last=Began,ProgressAt=Began,PhotoAt=0,WalkMetres=0,SavedYaw=0,SavedPitch=0,SavedClock=0,SavedFloor=0,RestoredGt=0;
  FVector LastFoot=FVector::ZeroVector,ProgressFoot=FVector::ZeroVector,SavedFoot=FVector::ZeroVector;TArray<FVector> Path;
  TArray<FBinding> Bindings;TMap<FString,bool> Doors;FJ NativeBefore,QuestBefore,BeforeSave,PhotoMeta;
  TArray<TSharedPtr<FJsonValue>> Executed,Events,Snapshots,Photos,Rays,Route;
  US47Main* Main()const{return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  FVector Target()const{return Web->Obj(TEXT("world.town.proxies.woman")).WorldPosition();}
  void Look()const
  {
   auto* Player=Main()->Player.Get();const FVector D=Target()-S47Three::ToThree(Player->EyeUE());Player->Yaw=FMath::Atan2(-D.X,-D.Z);Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  TArray<FString> ModalText()const
  {TArray<FString> Out;auto* M=Main();const auto Widget=M&&M->Hud?M->Hud->ModalWidget():TSharedPtr<SWidget>();if(Widget)TextOf(Widget.ToSharedRef(),Out);return Out;}
  void Result(bool Pass,const FString& Why)
  {
   const auto KeepAlive=AsShared();if(Done)return;Done=true;auto* M=Main();const auto FailedText=ModalText();
   if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(M&&M->Hud&&M->UI&&OwnedModal.IsValid()&&M->Hud->ModalWidget()==OwnedModal.Pin())M->UI->Close(true);
   if(Requested&&FScreenshotRequest::IsScreenshotRequested()&&FScreenshotRequest::GetFilename()==Root/(PhotoName+TEXT(".png")))FScreenshotRequest::Reset();
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("walk_metres"),WalkMetres);S47J::Set(O,TEXT("walk_samples"),WalkSamples);S47J::Set(O,TEXT("ordinary_OnUse_calls"),Uses);S47J::Set(O,TEXT("last_modal_text"),FailedText);S47J::Set(O,TEXT("pending_capture"),PhotoMeta);
   S47J::Set(O,TEXT("last_native_readback"),Web.IsValid()?Web->RadioWomanReadback():FJ());if(M&&M->Player){Vec(O,TEXT("final_player_three_m"),M->Player->Pos);S47J::Set(O,TEXT("final_clock_seconds"),M->Story?M->Story->Clock():0.);}
   O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("events"),Events);O->SetArrayField(TEXT("snapshots"),Snapshots);O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("interaction_rays"),Rays);O->SetArrayField(TEXT("physical_route_samples"),Route);
   S47J::Set(O,TEXT("scope"),TEXT("One explicit isolated chapter2 exterior foot start and 18:30 clock fixture. Ordinary continuous Walker, actual camera/focus and Player.OnUse open the original woman's modal. Caliche.Choice(radio) is an explicit API choice, verified through actual Slate lines and met/asked flags. Full unchanged case0 snapshot copied to isolated case1, fresh reader, ordinary title/Continue and a second actual OnUse prove consumed-topic persistence."));
   S47J::Set(O,TEXT("limits"),TEXT("No car, commute, human controller, listening, FPS or appearance acceptance. Existing separate commute/LOD/sole/schedule contract retains its own scope. No native pose modification, new quest, full matrix, installation or release. A closed dialogue is saved; an open modal is not claimed persistent."));
   const bool Written=Write(Root/TEXT("result.json"),O);if(Web.IsValid())Web->GetWorldTimerManager().ClearTimer(Timer);
   if(Pass&&Written){UE_LOG(LogS47RadioWomanConversation,Display,TEXT("S47: RadioWomanConversation PASS checks=%d walk=%.3fm evidence=%s"),Checks,WalkMetres,*Root);FPlatformMisc::RequestExitWithStatus(false,0);}
   else{UE_LOG(LogS47RadioWomanConversation,Error,TEXT("S47: RadioWomanConversation FAIL stage=%d: %s evidence=%s"),Stage,Written?*Why:TEXT("final result write failed"),*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Pass,const FString& Why)
  {
   ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("pass"),Pass);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("detail"),Why);S47J::Set(O,TEXT("at_seconds"),FPlatformTime::Seconds()-Began);Executed.Add(MakeShared<FJsonValueObject>(O));if(!Pass){Result(false,Why);return false;}return true;
  }
  void Next(int32 Value,const FString& Why)
  {Stage=Value;Since=FPlatformTime::Seconds();auto O=S47J::New();S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("detail"),Why);Events.Add(MakeShared<FJsonValueObject>(O));UE_LOG(LogS47RadioWomanConversation,Display,TEXT("S47 RadioWomanConversation stage %d: %s"),Stage,*Why);}
  bool CaptureBindings()
  {
   for(int32 I=0;I<Web->PartCount();++I)if(const auto* Node=Web->Node(I))if(Node->Name==TEXT("mannequin:radio-woman")){if(!Check(NativeNode<0,TEXT("one unique adopted native mannequin")))return false;NativeNode=I;}
   if(!Check(NativeNode>=0&&Web->Handle(TEXT("world.town.woman"))==1480&&Web->Handle(TEXT("world.town.proxies.woman"))==1544,TEXT("actual mannequin and unchanged woman1480/proxy1544 before fixture")))return false;
   for(int32 Index:{1480,1484,1485,1544,1545,NativeNode})
   {
    const auto* Node=Web->Node(Index);auto* Component=Node?Node->Comp.Get():nullptr;if(!Check(Node&&Component,TEXT("actual native/person/original housing/panel/proxy/leaf component exists")))return false;
    FBinding Row;Row.Node=Index;Row.Parent=Node->Parent;Row.Geo=Node->Geo;Row.Mats=Node->Mats;Row.Component=Component;Row.Transform=Component->GetComponentTransform();Row.Hidden=Component->bHiddenInGame;
    if(auto* Static=Cast<UStaticMeshComponent>(Component)){Row.Mesh=Static->GetStaticMesh().Get();Row.Collision=Static->GetCollisionEnabled();for(int32 Slot=0;Slot<Static->GetNumMaterials();++Slot)Row.Materials.Add(Static->GetMaterial(Slot));}
    Bindings.Add(Row);
   }
   return true;
  }
  bool Snapshot(const FString& Name)
  {
   auto* M=Main();auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("clock_seconds"),M->Story->Clock());S47J::Set(O,TEXT("area"),M->World->Area);Vec(O,TEXT("player_three_m"),M->Player->Pos);
   const auto Native=Web->RadioWomanReadback();S47J::Set(O,TEXT("native_readback"),Native);TArray<FString> Lines;const bool Audit=Web->AuditRadioWoman(Lines);S47J::Set(O,TEXT("native_audit_pass"),Audit);S47J::Set(O,TEXT("native_audit_lines"),Lines);
   const auto* Interaction=M->Inter->Get(TEXT("town:woman"));bool Intact=Bindings.Num()==6&&Interaction&&Interaction->Object.W==Web.Get()&&Interaction->Object.I==1544&&FMath::Abs(Interaction->Range-2.6)<1e-9&&!Interaction->bLook&&bool(Interaction->Use);
   TArray<TSharedPtr<FJsonValue>> Rows;
   for(const auto& Saved:Bindings)
   {
    const auto* Node=Web->Node(Saved.Node);auto* Component=Node?Node->Comp.Get():nullptr;auto Row=S47J::New();
    bool Good=Node&&Component&&Node->Comp==Saved.Component&&Node->Parent==Saved.Parent&&Node->Geo==Saved.Geo&&Node->Mats==Saved.Mats&&Component->GetComponentTransform().Equals(Saved.Transform,.00001)&&Component->bHiddenInGame==Saved.Hidden;
    S47J::Set(Row,TEXT("node"),Saved.Node);S47J::Set(Row,TEXT("component"),GetPathNameSafe(Component));S47J::Set(Row,TEXT("actual_component_object_id"),double(Component?Component->GetUniqueID():0));
    if(auto* Static=Cast<UStaticMeshComponent>(Component))
    {
     Good&=Static->GetStaticMesh()==Saved.Mesh.Get()&&Static->GetNumMaterials()==Saved.Materials.Num()&&Static->GetCollisionEnabled()==Saved.Collision;TArray<FString> Materials;
     for(int32 Slot=0;Slot<Static->GetNumMaterials();++Slot){Good&=Saved.Materials.IsValidIndex(Slot)&&Static->GetMaterial(Slot)==Saved.Materials[Slot].Get();Materials.Add(GetPathNameSafe(Static->GetMaterial(Slot)));}
     S47J::Set(Row,TEXT("actual_mesh"),GetPathNameSafe(Static->GetStaticMesh().Get()));S47J::Set(Row,TEXT("actual_materials"),Materials);
    }
    S47J::Set(Row,TEXT("bindings_unchanged"),Good);Intact&=Good;Rows.Add(MakeShared<FJsonValueObject>(Row));
   }
   O->SetArrayField(TEXT("actual_protected_bindings"),Rows);S47J::Set(O,TEXT("actual_interaction_range_m"),Interaction?Interaction->Range:-1.);S47J::Set(O,TEXT("actual_modal_text"),ModalText());Snapshots.Add(MakeShared<FJsonValueObject>(O));
   if(!Write(Root/(Name+TEXT(".json")),O)){Result(false,TEXT("native/schedule/proxy snapshot write failed"));return false;}
   if(!Check(Audit&&Native&&S47J::Bool(Native,TEXT("active"))&&S47J::Bool(Native,TEXT("visible"))&&S47J::Bool(Native,TEXT("static_only"))&&S47J::Bool(Native,TEXT("no_collision"))&&S47J::Int(Native,TEXT("native_node"))==NativeNode&&S47J::Int(Native,TEXT("radio_housing_triangles"))==12&&S47J::Int(Native,TEXT("radio_panel_triangles"))==12,TEXT("actual accepted static mannequin and preserved radio audit: ")+Name))return false;
   if(NativeBefore&&!Check(Same(NativeBefore,Native),TEXT("exact same native asset/geometry/seat/bounds/radio readback through conversation and full Continue")))return false;
   return Check(Intact&&M->TownLayer->Town->bWomanIn&&Web->ObjAt(1480).Shown()&&Web->ObjAt(1544).Shown()&&M->Inter->LabelOf(TEXT("town:woman"))==TEXT("Woman on the bench"),TEXT("original native/person/radio/panel/proxy bindings and real TownStep presence remain intact: ")+Name);
  }
  bool Floor()
  {
   auto* Player=Main()->Player.Get();FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(S47RadioWomanConversationFloor),true,Player);
   const bool Ground=Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Player->Pos.X,Player->FloorY+.12,Player->Pos.Z),S47Three::ToUE(Player->Pos.X,Player->FloorY-.35,Player->Pos.Z),ECC_Visibility,Query);
   return Check(Player->Walkable(Player->Pos.X,Player->Pos.Z)&&Ground&&Hit.ImpactNormal.Z>.7&&FMath::Abs(S47Three::ToThree(Hit.ImpactPoint).Y-Player->FloorY)<.08,TEXT("ordinary Walker and actual rendered exterior ground at route waypoint"));
  }
  bool Walk(double Now)
  {
   auto* Player=Main()->Player.Get();const double Moved=FVector2D(Player->Pos.X-LastFoot.X,Player->Pos.Z-LastFoot.Z).Size();if(Moved>FMath::Max(.5,(Now-Last)*4+.1)){Result(false,TEXT("nonphysical Walker discontinuity; no placement recovery"));return false;}
   WalkMetres+=Moved;LastFoot=Player->Pos;Last=Now;++WalkSamples;auto Sample=S47J::New();Vec(Sample,TEXT("player_three_m"),Player->Pos);S47J::Set(Sample,TEXT("walk_metres"),WalkMetres);Route.Add(MakeShared<FJsonValueObject>(Sample));
   if(FVector2D(Player->Pos.X-ProgressFoot.X,Player->Pos.Z-ProgressFoot.Z).Size()>.15){ProgressFoot=Player->Pos;ProgressAt=Now;}
   if(Waypoint>=Path.Num()){Player->TestMove.Reset();return true;}const FVector Delta=Path[Waypoint]-Player->Pos;const double Distance=FVector2D(Delta.X,Delta.Z).Size();
   if(Distance<.085){Player->TestMove.Reset();if(!Floor())return false;++Waypoint;return false;}if(Now-ProgressAt>12){Result(false,TEXT("ordinary bench approach stalled; no collision/pose fallback"));return false;}
   Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);Player->Pitch=-.08;Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.5));Player->bTestRun=false;return false;
  }
  bool UseWoman()
  {
   auto* M=Main();auto* Player=M->Player.Get();const auto* Interaction=M->Inter->Get(TEXT("town:woman"));const FVector Eye=Player->EyeUE(),Forward=Player->ForwardUE();const FString Pick=M->Inter->Update(Eye,Forward);
   auto Ray=S47J::New();Vec(Ray,TEXT("eye_ue_cm"),Eye);Vec(Ray,TEXT("forward_ue"),Forward);S47J::Set(Ray,TEXT("picked"),Pick);S47J::Set(Ray,TEXT("focused"),Player->FocusedId());S47J::Set(Ray,TEXT("walk_metres"),WalkMetres);
   TArray<FHitResult> Hits;FCollisionQueryParams Query(SCENE_QUERY_STAT(S47RadioWomanConversationRay),true);Web->GetWorld()->LineTraceMultiByChannel(Hits,Eye,Eye+Forward.GetSafeNormal()*600,ECC_GameTraceChannel1,Query);
   bool Actual=false;for(const auto& Hit:Hits)if(!Web->UsableAt(Hit.GetComponent()).IsEmpty())
   {
    S47J::Set(Ray,TEXT("first_usable_component"),GetPathNameSafe(Hit.GetComponent()));S47J::Set(Ray,TEXT("first_usable_node"),Web->NodeOf(Hit.GetComponent()));S47J::Set(Ray,TEXT("hit_distance_m"),Hit.Distance/100.);S47J::Set(Ray,TEXT("actual_range_m"),Interaction?Interaction->Range:-1.);
    Actual=Interaction&&Web->UsableAt(Hit.GetComponent())==TEXT("town:woman")&&Web->NodeOf(Hit.GetComponent())==1545&&Hit.Distance>0&&Hit.Distance/100.<=Interaction->Range;break;
   }
   Rays.Add(MakeShared<FJsonValueObject>(Ray));if(!Check(Actual&&Pick==TEXT("town:woman")&&Player->FocusedId()==Pick&&bool(Player->OnUse)&&!M->UI->Modal(),TEXT("actual settled camera/focus selects original woman proxy leaf within measured native use range before ordinary HUD OnUse")))return false;
   Player->OnUse(Pick);++Uses;if(M->Hud)OwnedModal=M->Hud->ModalWidget();return true;
  }
  bool Conversation(const FString& Kind)
  {
   auto* M=Main();const auto Text=ModalText();const FString Joined=FString::Join(Text,TEXT("\n"));bool Correct=M->UI->Modal()&&OwnedModal.IsValid()&&Joined.Contains(TEXT("LAUNDROMAT / THE WOMAN"))&&M->TownSeen.Contains(TEXT("met:radio_woman"));
   if(Kind==TEXT("first"))Correct&=Joined.Contains(TEXT("A transistor radio in her lap, tuned between two stations. Hiss, and under it something that is not quite a voice."))&&Joined.Contains(TEXT("It's louder tonight."))&&Joined.Contains(TEXT("She does not look at you when she says it."))&&Joined.Contains(TEXT("Ask what she is listening to"))&&!M->TownSeen.Contains(TEXT("ask:radio_woman:radio"));
   else if(Kind==TEXT("radio"))
   {
    for(const TCHAR* Line:{TEXT("What are you listening to?"),TEXT("Nothing. That's the point. In between."),TEXT("My husband said you could hear it in between, if you were quiet enough."),TEXT("Most nights it's just this. Tonight it's louder.")})Correct&=Joined.Contains(Line);
    Correct&=M->TownSeen.Contains(TEXT("ask:radio_woman:radio"))&&!Joined.Contains(TEXT("Ask what she is listening to"));
   }
   else Correct&=Kind==TEXT("again")&&Joined.Contains(TEXT("Still louder."))&&!Joined.Contains(TEXT("Ask what she is listening to"))&&M->TownSeen.Contains(TEXT("ask:radio_woman:radio"));
   auto Receipt=S47J::New();S47J::Set(Receipt,TEXT("kind"),Kind);S47J::Set(Receipt,TEXT("actual_slate_text"),Text);S47J::Set(Receipt,TEXT("met_flag"),M->TownSeen.Contains(TEXT("met:radio_woman")));S47J::Set(Receipt,TEXT("radio_asked_flag"),M->TownSeen.Contains(TEXT("ask:radio_woman:radio")));S47J::Set(Receipt,TEXT("pass"),Correct);
   if(!Write(Root/(TEXT("conversation-")+Kind+TEXT(".json")),Receipt)){Result(false,TEXT("actual conversation readback write failed"));return false;}return Check(Correct,TEXT("original woman modal/complete authored lines/available or consumed radio topic/flags: ")+Kind);
  }
  void QueuePhoto(const FString& Name){PhotoName=Name;Requested=false;PhotoAt=FPlatformTime::Seconds();PhotoMeta.Reset();}
  bool Photo()
  {
   Look();const double Now=FPlatformTime::Seconds();const FString File=Root/(PhotoName+TEXT(".png"));auto* M=Main();
   if(!Requested)
   {
    if(Now-PhotoAt<.6)return false;if(!Check(!FScreenshotRequest::IsScreenshotRequested()&&!IFileManager::Get().FileExists(*File),TEXT("own fresh viewport request without competing screenshot")))return false;
    PhotoMeta=S47J::New();S47J::Set(PhotoMeta,TEXT("name"),PhotoName);Vec(PhotoMeta,TEXT("player_three_m"),M->Player->Pos);Vec(PhotoMeta,TEXT("eye_ue_cm"),M->Player->EyeUE());Vec(PhotoMeta,TEXT("forward_ue"),M->Player->ForwardUE());S47J::Set(PhotoMeta,TEXT("actual_modal_text"),ModalText());S47J::Set(PhotoMeta,TEXT("frame"),double(GFrameCounter));
    if(auto* Controller=Cast<APlayerController>(M->Player->GetController()))if(const auto* Camera=Controller->PlayerCameraManager.Get()){Vec(PhotoMeta,TEXT("camera_manager_ue_cm"),Camera->GetCameraLocation());S47J::Set(PhotoMeta,TEXT("fov_degrees"),double(Camera->GetFOVAngle()));}
    FScreenshotRequest::RequestScreenshot(File,true,false);Requested=true;return false;
   }
   FJ Receipt;if(!Png(File,Receipt)){if(Now-PhotoAt>15)Result(false,TEXT("complete PNG CRC/IEND missing: ")+PhotoName);return false;}S47J::Set(PhotoMeta,TEXT("png"),Receipt);Photos.Add(MakeShared<FJsonValueObject>(PhotoMeta));
   if(!Write(Root/(PhotoName+TEXT(".capture.json")),PhotoMeta)){Result(false,TEXT("capture metadata write failed"));return false;}Requested=false;return true;
  }
  bool DialogSame(const FJ& A,const FJ& B)
  {for(const TCHAR* Key:{TEXT("pack"),TEXT("radioPack"),TEXT("takeaway")})if(!Same(S47J::Obj(S47J::Obj(A,TEXT("evening")),Key),S47J::Obj(S47J::Obj(B,TEXT("evening")),Key)))return false;return true;}
  void Tick()
  {
   const auto KeepAlive=AsShared();if(Done)return;const double Now=FPlatformTime::Seconds();auto* M=Main();if(Now-Began>150){Result(false,TEXT("150 second targeted conversation timeout; unexecuted phases not accepted"));return;}
   if(!M||!M->Flow||!M->Story||!M->World||!M->Player||!M->UI||!M->Inter||!M->Evening||!M->TownLayer||!M->TownLayer->Town||!M->TownLayer->Caliche||S47Startup::Blocking()){Ready=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){Ready=0;return;}if(Ready==0){Ready=Now;return;}if(Now-Ready<2)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Flow->Saves.List().IsEmpty()&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()))&&M->Story->ChapterKey==TEXT("ch2")&&!M->Story->bEveningTimeline&&!M->Evening->Opening()&&!M->World->bDriving&&!M->UI->Modal()&&!M->Story->IsCinematic()&&!M->TownSeen.Contains(TEXT("met:radio_woman"))&&!M->TownSeen.Contains(TEXT("ask:radio_woman:radio")),TEXT("fresh isolated chapter2/case0 before sole declared foot/clock fixture, no old conversation or commute")))return;
    if(!CaptureBindings())return;QuestBefore=Quest(M->Flow->GameState());if(!Check(QuestBefore.IsValid(),TEXT("actual complete chapter2 quest case before fixture")))return;
    auto Fixture=S47J::New();S47J::Set(Fixture,TEXT("kind"),TEXT("EXPLICIT FOOT+CLOCK FIXTURE ONLY; no car, egress, natural arrival or earlier chronology acceptance"));Vec(Fixture,TEXT("foot_three_x_y_z"),FVector(7991,0,2138));S47J::Set(Fixture,TEXT("clock_before"),M->Story->Clock());S47J::Set(Fixture,TEXT("fixture_clock_seconds"),-19800.);
    if(!Write(Root/TEXT("explicit-foot-clock-fixture.json"),Fixture)){Result(false,TEXT("fixture provenance write failed"));return;}Started=true;M->World->Enter(TEXT("diner"));M->Player->Place(7991,2138,0);M->Story->SetClock(-19800);Next(1,TEXT("one explicit exterior foot start and 18:30 clock; ordinary TownStep supplies presence"));return;
   }
   if(M->Flow->bStarting||M->World->bBusy)return;if(Stage!=9&&M->Mode!=US47Main::EMode::Play){Result(false,TEXT("unexpected mode outside title/Continue stage"));return;}
   if(Stage==1)
   {
    if(Now-Since<.8)return;if(!Floor()||!Snapshot(TEXT("arrival-before-conversation")))return;NativeBefore=S47J::Clone(Web->RadioWomanReadback());
    const FVector Point=Target();Path={FVector(Point.X+2.05,0,2138),FVector(Point.X+2.05,0,Point.Z)};LastFoot=ProgressFoot=M->Player->Pos;Last=ProgressAt=Now;Waypoint=0;
    for(const auto& Position:Path)if(!Check(M->Player->Walkable(Position.X,Position.Z),TEXT("declared bench waypoints lie in original legal exterior walk area")))return;Next(2,TEXT("ordinary continuous Walker approach to existing woman proxy"));return;
   }
   if(Stage==2){if(!Walk(Now))return;Look();QueuePhoto(TEXT("physical-bench-approach"));Next(3,TEXT("actual camera at bench before HUD interaction"));return;}
   if(Stage==3){if(!Photo())return;if(!Check(WalkMetres>6&&WalkSamples>40,TEXT("real continuous native Walker approach precedes any conversation"))||!UseWoman())return;Next(4,TEXT("ordinary Player.OnUse opens original woman's first conversation"));return;}
   if(Stage==4)
   {
    if(Now-Since<.3)return;if(!Conversation(TEXT("first"))||!Snapshot(TEXT("first-dialogue")))return;QueuePhoto(TEXT("first-woman-modal"));Next(5,TEXT("capture complete actual initial dialogue before API topic choice"));return;
   }
   if(Stage==5){if(!Photo())return;M->TownLayer->Caliche->Choice(TEXT("radio"));Next(6,TEXT("explicit ordinary Caliche.Choice radio, not a manual state flag"));return;}
   if(Stage==6)
   {
    if(Now-Since<.3)return;if(!Conversation(TEXT("radio"))||!Snapshot(TEXT("radio-topic-dialogue")))return;QueuePhoto(TEXT("radio-topic-modal"));Next(7,TEXT("actual radio-topic lines and committed met/asked state"));return;
   }
   if(Stage==7)
   {
    if(!Photo())return;M->UI->Close();OwnedModal.Reset();Next(8,TEXT("ordinary close; save only at a legal closed-dialogue boundary"));return;
   }
   if(Stage==8)
   {
    if(Now-Since<.8)return;if(!Check(!M->UI->Modal()&&M->Flow->GameSaveBlock().IsEmpty()&&Same(QuestBefore,Quest(M->Flow->GameState())),TEXT("conversation changed no chapter/board quest and finished at legal closed-dialogue save boundary")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedFloor=M->Player->FloorY;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();Doors=Web->DoorStates();
    if(!Check(S47J::Str(BeforeSave,TEXT("checkpoint"))==TEXT("chapter2"),TEXT("actual chapter2 checkpoint retained")))return;
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=M->Story->ChapterTitle();Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;FString Error;
    const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT DISK FIXTURE: unchanged complete actual case0 snapshot stored as isolated case1: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),BeforeSave),TEXT("fresh reader equals entire real pre-title disk snapshot")))return;if(!Write(Root/TEXT("before-continue.disk.json"),BeforeSave)){Result(false,TEXT("pre-title disk evidence write failed"));return;}
    SaveId=Meta->Id;M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(9,TEXT("ordinary title and exact isolated disk Continue"));return;
   }
   if(Stage==9)
   {
    if(Now-Since<.6)return;const auto Meta=M->Flow->Saves.ContinueSave();if(!Check(M->Mode==US47Main::EMode::Title&&Meta&&Meta->Id==SaveId,TEXT("ordinary title resolves exact isolated disk save")))return;
    if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary Continue action accepted")))return;RestoredGt=M->Story->Gt();
    if(!Check(M->Mode==US47Main::EMode::Play&&!M->Flow->bStarting&&FMath::Abs(M->Story->Clock()-SavedClock)<.001,TEXT("exact saved native clock immediately after full synchronous Continue before next frame")))return;Next(10,TEXT("complete OpenSave/StartGame, never direct subsystem restore"));return;
   }
   if(Stage==10)
   {
    if(Now-Since<1||!M->Flow->SaveBlock().IsEmpty())return;const auto Actual=M->Flow->GameState();const double ClockDelta=M->Story->Clock()-SavedClock,GameDelta=M->Story->Gt()-RestoredGt;
    bool Good=M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->Story->ChapterKey==TEXT("ch2")&&M->World->Area==TEXT("diner")&&!M->World->bDriving&&M->Player->Pos.Equals(SavedFoot,.015)&&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Player->FloorY-SavedFloor)<.08&&ClockDelta>=0&&FMath::Abs(ClockDelta-GameDelta)<.025&&SameDoors(Doors,Web->DoorStates())&&Same(Quest(BeforeSave),Quest(Actual))&&DialogSame(BeforeSave,Actual)&&!M->UI->Modal();
    for(const auto& Key:S47J::Strs(BeforeSave,TEXT("town")))Good&=M->TownSeen.Contains(Key);Good&=M->TownSeen.Contains(TEXT("met:radio_woman"))&&M->TownSeen.Contains(TEXT("ask:radio_woman:radio"));
    auto Receipt=S47J::New();S47J::Set(Receipt,TEXT("expected"),BeforeSave);S47J::Set(Receipt,TEXT("actual"),Actual);S47J::Set(Receipt,TEXT("clock_delta_seconds"),ClockDelta);S47J::Set(Receipt,TEXT("actual_native_game_time_since_continue_seconds"),GameDelta);S47J::Set(Receipt,TEXT("pass"),Good);
    if(!Write(Root/TEXT("continue-readback.json"),Receipt)){Result(false,TEXT("full Continue readback write failed"));return;}
    if(!Check(Good,TEXT("full chapter2 Continue retained actual area/player/view/floor/clock/doors/whole quest/dialogue packs and all conversation keys"))||!Snapshot(TEXT("after-continue")))return;Look();Next(11,TEXT("fresh restored actual camera before ordinary OnUse again"));return;
   }
   if(Stage==11){Look();if(Now-Since<.35)return;if(!UseWoman())return;Next(12,TEXT("ordinary OnUse again after disk Continue, no direct Caliche.Talk or Inter.UseId"));return;}
   if(Stage==12)
   {
    if(Now-Since<.3)return;if(!Conversation(TEXT("again"))||!Snapshot(TEXT("after-continue-again-dialogue")))return;QueuePhoto(TEXT("after-continue-woman-modal"));Next(13,TEXT("return dialogue and already-consumed radio topic visible after actual disk restore"));return;
   }
   if(Stage==13){if(!Photo())return;M->UI->Close();OwnedModal.Reset();Next(14,TEXT("ordinary close before final normal post-Continue save"));return;}
   if(Stage==14)
   {
    if(Now-Since<.8)return;const auto Expected=M->Flow->GameState();FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary case1 Flow.WriteSave after restored conversation: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Actual=Reader.Load(Meta->Id);if(!Check(Same(Expected,Actual),TEXT("fresh reader equals entire ordinary post-Continue save")))return;if(!Write(Root/TEXT("after-continue-normal-save.disk.json"),Actual)){Result(false,TEXT("final ordinary disk evidence write failed"));return;}Next(15,TEXT("required viewport and complete executed-phase evidence before PASS"));return;
   }
   if(Stage==15)
   {
    if(!Check(Photos.Num()==4&&Uses==2&&M->TownSeen.Contains(TEXT("met:radio_woman"))&&M->TownSeen.Contains(TEXT("ask:radio_woman:radio")),TEXT("all physical/modal/topic/Continue phases and actual conversation progress completed")))return;
    for(const auto& Value:Photos){const auto Old=S47J::Obj(Value->AsObject(),TEXT("png"));FJ Fresh;if(!Check(Png(S47J::Str(Old,TEXT("file")),Fresh)&&Same(Old,Fresh),TEXT("final unchanged viewport PNG signature/chunk CRC/IEND/SHA receipt")))return;}
    Result(true,TEXT("physical chapter2 bench approach and actual HUD OnUse, original radio dialogue/topic progression, preserved static native/radio/proxy and full isolated disk/title/Continue"));
   }
  }
 };
}

void S47BeginRadioWomanConversationContract(AS47Web* Web)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanConversation")))return;
 FString UserDir,Jump,Value;const bool HasDir=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);const bool RawSafe=HasDir&&!UserDir.Contains(TEXT(".."));UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 const bool Safe=Web&&RawSafe&&(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir);
 if(!Safe){UE_LOG(LogS47RadioWomanConversation,Error,TEXT("S47: RadioWomanConversation FAIL requires explicit isolated /tmp UserDir; no personal Saved"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto State=MakeShared<S47RadioWomanConversation::FControl>();State->Web=Web;const FString Base=FPaths::ProjectSavedDir()/TEXT("RadioWomanConversation");const bool Fresh=!IFileManager::Get().DirectoryExists(*Base);State->Root=Base/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*State->Root,true)){UE_LOG(LogS47RadioWomanConversation,Error,TEXT("S47: RadioWomanConversation FAIL evidence directory"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 bool Conflict=false;
 for(const TCHAR* Key:{TEXT("S47Case="),TEXT("S47Evening="),TEXT("S47EveningRoute="),TEXT("S47Drive="),TEXT("S47Walk="),TEXT("S47WalkFrom="),TEXT("S47Face="),TEXT("S47Auto="),TEXT("S47Hide="),TEXT("S47ProductionSuite="),TEXT("S47Open="),TEXT("S47HorrorContract=")})Conflict|=FParse::Value(FCommandLine::Get(),Key,Value);
 for(const TCHAR* Flag:{TEXT("NullRHI"),TEXT("S47Title"),TEXT("S47Evening"),TEXT("S47SmokeTest"),TEXT("S47ProductionViews"),TEXT("S47ProductionCab"),TEXT("S47FarmAccessContract"),TEXT("S47CalicheHomesContract"),TEXT("S47CalicheHomesContractFallback"),TEXT("S47CinemaPostersContract"),TEXT("S47RadioWomanContract"),TEXT("S47RadioWomanOff"),TEXT("S47DaleDocumentsContract"),TEXT("S47MotelV2Route"),TEXT("S47NightSequenceContract"),TEXT("S47T01DriveContract"),TEXT("S47T01Contract"),TEXT("S47T02Contract"),TEXT("S47T02DriveContract"),TEXT("S47HorrorArrival"),TEXT("S47HorrorContract"),TEXT("S47GetOut"),TEXT("S47EveningEntrance"),TEXT("S47PropsOff"),TEXT("S47EveningTimeline")})Conflict|=FParse::Param(FCommandLine::Get(),Flag);
 if(!State->Check(Fresh&&!Conflict&&FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump)&&Jump==TEXT("chapter2"),TEXT("fresh empty chapter2/case0 profile with normal native/RHI and no competing pose/route/clock driver")))return;
 Web->GetWorldTimerManager().SetTimer(State->Timer,[State]{State->Tick();},.05f,true);
 UE_LOG(LogS47RadioWomanConversation,Display,TEXT("S47: RadioWomanConversation armed; explicit chapter2 foot/18:30 fixture, actual Walker/OnUse and disk Continue; no car claim; evidence=%s"),*State->Root);
}
