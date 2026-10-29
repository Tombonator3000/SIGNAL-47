#include "S47SC11Contract.h"
#include "S47SC11.h"
#include "S47Chapter4.h"
#include "S47Docs.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 void SC11Text(const TSharedRef<SWidget>& Widget,TArray<FString>& Out)
 {
  if(!Widget->GetVisibility().IsVisible())return;
  if(Widget->GetTypeAsString()==TEXT("STextBlock"))Out.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children=Widget->GetChildren();
  for(int32 Index=0;Children&&Index<Children->Num();++Index)SC11Text(Children->GetChildAt(Index),Out);
 }
 bool SC11Png(const FString& Path,S47J::FObj& Out)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return false;
  const uint8 Signature[]={137,80,78,71,13,10,26,10};
  if(Bytes.Num()<33||FMemory::Memcmp(Bytes.GetData(),Signature,8))return false;
  const auto U32=[&](int64 At){return(uint32(Bytes[At])<<24)|(uint32(Bytes[At+1])<<16)|(uint32(Bytes[At+2])<<8)|Bytes[At+3];};
  int64 At=8,Data=0;int32 Chunks=0;uint32 Width=0,Height=0;bool End=false;
  while(At+12<=Bytes.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>Bytes.Num()-At-12)return false;const uint8* Type=Bytes.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(!Chunks){if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13)return false;Width=U32(At+8);Height=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Data+=Size;++Chunks;At+=int64(Size)+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=Bytes.Num()||!Data||Width<320||Height<200)return false;
  FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);Out=S47J::New();
  S47J::Set(Out,TEXT("path"),Path);S47J::Set(Out,TEXT("bytes"),Bytes.Num());S47J::Set(Out,TEXT("sha1"),Hash.ToString());
  S47J::Set(Out,TEXT("width"),double(Width));S47J::Set(Out,TEXT("height"),double(Height));S47J::Set(Out,TEXT("crc_iend_eof"),true);return true;
 }
}

FS47SC11* FS47SC11Contract::Act() const{return M&&M->Spine?M->Spine->Siege():nullptr;}
void FS47SC11Contract::Next(int32 Value,double Wait){Stage=Value;Hold=Wait;Mark=FPlatformTime::Seconds();}

void FS47SC11Contract::Finish(bool Good,const FString& Why)
{
 if(Done)return;Done=true;
 if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 auto Out=S47J::New();S47J::Set(Out,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(Out,TEXT("mode"),Mode);
 S47J::Set(Out,TEXT("stage"),Stage);S47J::Set(Out,TEXT("reason"),Why);S47J::Set(Out,TEXT("checks"),Checks);S47J::Set(Out,TEXT("actual_ray_uses"),Uses);
 S47J::Set(Out,TEXT("stores"),Stores);S47J::Set(Out,TEXT("walked_metres"),Walked);S47J::Set(Out,TEXT("walk_samples"),WalkSamples);
 S47J::Set(Out,TEXT("normal_title_continue"),NormalContinue);S47J::Set(Out,TEXT("actual_body_render_witness"),RenderWitness);
 S47J::Set(Out,TEXT("requested_picture_paths"),Pictures);S47J::Set(Out,TEXT("final_facts"),Act()?Act()->Save():S47J::FObj());
 S47J::Set(Out,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(Out,TEXT("method"),TEXT("Declared chapter4/prior-night and OnEnd fixture; one walkable SARO start. All subsequent player movement is native Walker input/Resolve, all world uses actual camera ray/focus/HUD OnUse. Actual local intensity, three moving spotlights, room observation, physical escape/capture and fresh disk/full Flow/title Continue."));
 S47J::Set(Out,TEXT("limits"),TEXT("SC01-SC10 and the return drive are explicit fixtures, not replayed. Escape is the actual truck-pad approach boundary, not boarding/driving. Closed-door query is a named reversible geometry fixture; opacity/depth flags and LOS are not pixel-based visual acceptance. API input is not human input; PNG integrity is not visual/FPS acceptance."));
 Out->SetArrayField(TEXT("executed_checks"),Executed);Out->SetArrayField(TEXT("rays"),Rays);Out->SetArrayField(TEXT("states"),States);
 Out->SetArrayField(TEXT("restores"),Restores);Out->SetArrayField(TEXT("sight_queries"),Sight);Out->SetArrayField(TEXT("photos"),Photos);
 const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(Out,true),*(Root/TEXT("result.json")));
 if(Good&&Written)UE_LOG(LogTemp,Display,TEXT("S47: spine contract %s PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm evidence=%s"),*Mode,Checks,Stores,Uses,Walked,*Root);
 else UE_LOG(LogTemp,Error,TEXT("S47: spine contract %s FAIL stage=%d: %s (result written %d)"),*Mode,Stage,*Why,Written);
 FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:1);
}
bool FS47SC11Contract::Check(bool Good,const FString& Why)
{
 ++Checks;auto Row=S47J::New();S47J::Set(Row,TEXT("check"),Checks);S47J::Set(Row,TEXT("stage"),Stage);S47J::Set(Row,TEXT("pass"),Good);S47J::Set(Row,TEXT("detail"),Why);
 Executed.Add(MakeShared<FJsonValueObject>(Row));if(!Good){Finish(false,Why);return false;}
 UE_LOG(LogTemp,Display,TEXT("S47: SC11 PASS: %s"),*Why);return true;
}
void FS47SC11Contract::Path(TArray<FVector2D> Points)
{
 Route=MoveTemp(Points);Cursor=0;LastWalk=M->Player->Pos;Mark=FPlatformTime::Seconds();
}
bool FS47SC11Contract::Walk(bool Run)
{
 auto* Player=M->Player.Get();const double Distance=FVector2D(Player->Pos.X-LastWalk.X,Player->Pos.Z-LastWalk.Z).Size();
 if(Distance>.8){Finish(false,TEXT("Walker discontinuity; no route placement recovery"));return false;}
 Walked+=Distance;++WalkSamples;LastWalk=Player->Pos;
 if(FPlatformTime::Seconds()-Mark>24){Finish(false,TEXT("native Walker stalled before ")+Route[Cursor].ToString());return false;}
 const FVector2D Delta=Route[Cursor]-FVector2D(Player->Pos.X,Player->Pos.Z);
 if(Delta.Size()<.10){Player->TestMove.Reset();++Cursor;Mark=FPlatformTime::Seconds();if(Cursor==Route.Num())return true;return false;}
 Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Y);Player->Pitch=-.04;
 Player->TestMove=FVector2D(0,-FMath::Clamp(Delta.Size()/.5,0.,1.));Player->bTestRun=Run;return false;
}
void FS47SC11Contract::FacePoint(const FVector& Three)
{
 Aim=S47Three::ToUE(Three);const FVector Eye=S47Three::ToThree(M->Player->EyeUE()),Delta=Three-Eye;
 M->Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);M->Player->Pitch=FMath::Atan2(Delta.Y,FVector2D(Delta.X,Delta.Z).Size());
}
bool FS47SC11Contract::Face(const FString& Id)
{
 const auto* Item=M->Inter->Get(Id);FS47Obj Object=Item?Item->Object:FS47Obj();
 if(!Object)if(const auto* WebItem=M->Web->Item(Id))Object=M->Web->ObjAt(WebItem->Node);
 const FBox Box=Object.WorldBox();if(!Check(Object&&Box.IsValid,TEXT("original interaction geometry: ")+Id))return false;
 if(!Check(!OriginalNodes.Contains(Id)||OriginalNodes[Id]==Object.I,TEXT("same original interaction node: ")+Id))return false;
 OriginalNodes.Add(Id,Object.I);FacePoint(Box.GetCenter());return true;
}
bool FS47SC11Contract::Use(const FString& Id)
{
 const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Picked=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);
 auto Row=S47J::New();S47J::Set(Row,TEXT("id"),Id);S47J::Set(Row,TEXT("picked"),Picked);S47J::Set(Row,TEXT("focused"),Focus);
 S47J::Set(Row,TEXT("eye_ue_cm"),Eye.ToString());S47J::Set(Row,TEXT("forward_ue"),Forward.ToString());S47J::Set(Row,TEXT("target_distance_m"),FVector::Dist(Eye,Aim)/100.);
 S47J::Set(Row,TEXT("actual_range_m"),M->Inter->RangeOf(Id));Rays.Add(MakeShared<FJsonValueObject>(Row));
 if(!Check(Picked==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary HUD OnUse: ")+Id))return false;
 M->Player->OnUse(Picked);++Uses;return true;
}
bool FS47SC11Contract::Document(const FString& Id,const FString& Fragment)
{
 const FS47Doc* Doc=M->Story->Docs().FindByPredicate([&](const FS47Doc& Value){return Value.Id==Id;});
 TArray<FString> Text;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)SC11Text(Modal.ToSharedRef(),Text);
 const bool Visible=Text.ContainsByPredicate([&](const FString& Value){return Value.Contains(Fragment);});
 if(!Check(Doc&&Doc->Page.Contains(Fragment)&&M->UI->Modal()&&Visible,TEXT("actual original document and visible modal: ")+Id))return false;
 SourcePages.Add(Id,Doc->Page);
 M->UI->Close(true);return true;
}
bool FS47SC11Contract::Sources(const FString& Context)
{
 FS47SC11* A=Act();if(!Check(A!=nullptr,Context+TEXT(" live siege owner")))return false;
 for(const auto& Entry:OriginalDocs)
  if(!Check(M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==Entry.Key&&D.Page==Entry.Value;}),Context+TEXT(" preceding original preserved: ")+Entry.Key))return false;
 const struct {const TCHAR* Id;bool Have;} Need[]={
  {TEXT("HR_SARO_RX_SECURED"),A->F.Receiver},{TEXT("HR_SARO_EVENT_STARTED"),A->F.Event},{TEXT("HR_HONDO_REMOTE"),A->F.Hondo},{TEXT("HR_SITE11_REMOTE"),A->F.Site11},
  {TEXT("HR_SARO_ORIGINAL"),A->F.Escaped}};
 for(const auto& Item:Need)
 {
  const FS47Doc* D=M->Story->Docs().FindByPredicate([&](const FS47Doc& Value){return Value.Id==Item.Id;});
  if(!Check(bool(D)==Item.Have,Context+TEXT(" source presence, no fabricated escape: ")+Item.Id))return false;
  if(D&&SourcePages.Contains(Item.Id)&&!Check(D->Page==SourcePages[Item.Id],Context+TEXT(" exact collected source text preserved: ")+Item.Id))return false;
  if(D&&FString(Item.Id)==TEXT("HR_SARO_ORIGINAL")&&!Check(D->Page==FS47SC11::ReceiverDoc(true,A->EscapedAt).Page,Context+TEXT(" completed source binds actual stored escape time")))return false;
  if(D&&FString(Item.Id)!=TEXT("HR_SARO_ORIGINAL")&&!Check(!D->Page.Contains(TEXT("WORLD ESCAPE RECORDED")),Context+TEXT(" partial source never claims escape")))return false;
 }
 return true;
}
void FS47SC11Contract::Snapshot(const FString& Name)
{
 auto Row=S47J::New();S47J::Set(Row,TEXT("name"),Name);S47J::Set(Row,TEXT("clock"),M->Story->Clock());S47J::Set(Row,TEXT("area"),M->World->Area);
 S47J::Set(Row,TEXT("player_three_m"),M->Player->Pos.ToString());S47J::Set(Row,TEXT("doors"),M->Web->DoorStates().Num());
 S47J::Set(Row,TEXT("camera_eye_ue_cm"),M->Player->EyeUE().ToString());S47J::Set(Row,TEXT("camera_forward_ue"),M->Player->ForwardUE().ToString());
 if(FS47SC11* A=Act())
 {
  S47J::Set(Row,TEXT("facts"),A->Save());S47J::Set(Row,TEXT("local_intensity"),A->LocalLamp?A->LocalLamp.Intensity():-1);
  S47J::Set(Row,TEXT("body_at_three_xz"),A->Pos.ToString());S47J::Set(Row,TEXT("body_state"),int32(A->B));
  S47J::Set(Row,TEXT("body_shown"),A->bShown);S47J::Set(Row,TEXT("recent_torso_render"),A->Pale.Rendered());
  TArray<TSharedPtr<FJsonValue>> Lights;
  for(int32 Index=0;Index<A->Beams.Num();++Index)if(const auto* Light=A->Beams[Index].Get())
  {
   auto L=S47J::New();S47J::Set(L,TEXT("component"),Light->GetPathName());S47J::Set(L,TEXT("visible"),Light->IsVisible());
   S47J::Set(L,TEXT("intensity"),double(Light->Intensity));S47J::Set(L,TEXT("origin_ue_cm"),Light->GetComponentLocation().ToString());
   S47J::Set(L,TEXT("forward_ue"),Light->GetForwardVector().ToString());S47J::Set(L,TEXT("cone_deg"),double(Light->OuterConeAngle));
   const FVector ToEye=M->Player->EyeUE()-Light->GetComponentLocation();
   S47J::Set(L,TEXT("eye_distance_cm"),ToEye.Size());S47J::Set(L,TEXT("forward_dot_eye"),FVector::DotProduct(Light->GetForwardVector(),ToEye.GetSafeNormal()));
   S47J::Set(L,TEXT("actual_eye_line_clear"),A->ClearSight(S47Three::ToThree(Light->GetComponentLocation()),S47Three::ToThree(M->Player->EyeUE())));
   S47J::Set(L,TEXT("casts_shadow"),bool(Light->CastShadows));S47J::Set(L,TEXT("actual_beam_sees_player"),A->BeamSeesPlayer(Index));Lights.Add(MakeShared<FJsonValueObject>(L));
  }
  Row->SetArrayField(TEXT("actual_spotlights"),Lights);
 }
 States.Add(MakeShared<FJsonValueObject>(Row));
}
bool FS47SC11Contract::Restore(const FString& Name)
{
 M->Player->TestMove.Reset();
 if(M->Story->IsCinematic()||M->World->bBusy||M->Flow->bStarting)
 {if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("save checkpoint still cinematic/busy: ")+Name);return false;}
 M->UI->Close(true);const FString Block=M->Flow->GameSaveBlock();
 if(!Block.IsEmpty()||M->Story->IsCinematic()||M->World->bBusy||M->ModalOpen())
 {if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("ordinary gameplay save gate: ")+Name+TEXT(" / ")+Block);return false;}
 Expected=S47J::Clone(M->Flow->GameState());
 FS47SaveStore Writer(Root/TEXT("cohorts")/Name);Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("SC11");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(true);
 FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,Expected,Card,Error);
 if(!Check(Meta.IsSet(),TEXT("isolated complete disk write: ")+Name+TEXT(" / ")+Error))return false;
 FS47SaveStore Reader(Writer.Dir);Reader.Init();const auto Disk=Reader.Load(Meta->Id);
 if(!Check(Disk&&S47J::Write(Disk)==S47J::Write(Expected),TEXT("independent disk reader: ")+Name))return false;
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("retain full raw disk record: ")+Name))return false;
 ++Stores;Pending=Name;US47Flow::FStart Start;Start.State=Disk;Start.CaseId=0;Start.bDev=true;Start.Playtime=M->Playtime;M->Flow->StartGame(Start);return true;
}
bool FS47SC11Contract::Restored()
{
 FS47SC11* A=Act();const auto Spine=S47J::Obj(Expected,TEXT("spine")),Saved=S47J::Obj(Spine,TEXT("sc11d")),Case=S47J::Obj(Expected,TEXT("case")),Pose=S47J::Obj(Expected,TEXT("pose"));
 if(!Check(A&&M->Story->bEveningTimeline&&S47J::Write(A->Save())==S47J::Write(Saved)&&M->Spine->Gate(FS47Spine::SC11)==S47J::Int(Spine,TEXT("sc11")),TEXT("full Flow retains facts/gate without fabricated escape: ")+Pending))return false;
 const bool Hunting=A->HuntLive();
 if(!Check(M->Story->Clock()>=S47J::Num(Case,TEXT("clock"))&&M->Story->Clock()-S47J::Num(Case,TEXT("clock"))<4
  &&M->World->Area==S47J::Str(Expected,TEXT("area"))&&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.04&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.04
  &&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001
  &&S47J::Write(S47J::Obj(M->Flow->GameState(),TEXT("doors")))==S47J::Write(S47J::Obj(Expected,TEXT("doors"))),TEXT("disk retains clock/area/player/yaw/doors; hunt explicitly restarts locally: ")+Pending))return false;
 if(Hunting&&!Check(!A->F.Escaped&&A->T<3&&A->B==FS47SC11::EBody::Approach,TEXT("saved hunt resumes the local approach, not a free escape")))return false;
 if(!Sources(TEXT("restored ")+Pending))return false;
 for(const auto& Item:OriginalNodes)
 {
  const auto* Interaction=M->Inter->Get(Item.Key);const auto* WebItem=M->Web->Item(Item.Key);
  const int32 Node=Interaction&&Interaction->Object?Interaction->Object.I:WebItem?WebItem->Node:-1;
  if(!Check(Node==Item.Value,TEXT("original physical interaction identity survives full restore: ")+Item.Key))return false;
 }
 auto Receipt=S47J::New();S47J::Set(Receipt,TEXT("name"),Pending);S47J::Set(Receipt,TEXT("pass"),true);S47J::Set(Receipt,TEXT("actual_sc11"),A->Save());
 S47J::Set(Receipt,TEXT("clock"),M->Story->Clock());Restores.Add(MakeShared<FJsonValueObject>(Receipt));Pending.Reset();return true;
}
bool FS47SC11Contract::SightChecks()
{
 FS47SC11* A=Act();const bool Was=M->Web->IsDoorOpen(TEXT("doorSouth"));
 // Reversible query fixture, before the hunt. No player/body placement or accepted route claim.
 M->Web->SetDoorOpen(TEXT("doorSouth"),false,true);
 const FVector From(-2.4,1.7,3.1),To(-2.4,1.7,5.7),WallA(-.5,1.7,3.8),WallB(-.5,1.7,5.7);
 const bool DoorClear=A->ClearSight(From,To),WallClear=A->ClearSight(WallA,WallB);
 FHitResult Hit;const bool Block=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(From),S47Three::ToUE(To),ECC_Visibility);
 M->Web->SetDoorOpen(TEXT("doorSouth"),true,true);const bool OpenClear=A->ClearSight(From,To);
 auto Row=S47J::New();S47J::Set(Row,TEXT("method"),TEXT("Reversible actual closed/open door geometry query, prior to hunt; original state restored"));
 S47J::Set(Row,TEXT("door_clear"),DoorClear);S47J::Set(Row,TEXT("opaque_wall_clear"),WallClear);S47J::Set(Row,TEXT("first_hit"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():TEXT("none"));
 S47J::Set(Row,TEXT("same_query_open_door_clear"),OpenClear);
 S47J::Set(Row,TEXT("first_hit_node"),M->Web->NodeOf(Hit.GetComponent()));S47J::Set(Row,TEXT("door_original_open"),Was);Sight.Add(MakeShared<FJsonValueObject>(Row));
 M->Web->SetDoorOpen(TEXT("doorSouth"),Was,true);
 return Check(!DoorClear&&OpenClear&&!WallClear&&Block,TEXT("actual closed door/opaque wall reject LOS; same open doorway permits it"));
}
bool FS47SC11Contract::RenderChecks()
{
 FS47SC11* A=Act();if(!Check(A&&A->Pale.bBuilt&&A->Pale.All.Num()==10&&A->Pale.Rendered(),TEXT("actual ten-part pale body has rendered from this camera")))return false;
 TArray<TSharedPtr<FJsonValue>> Parts;TSet<FString> Meshes;
 for(const auto& Weak:A->Pale.All)
 {
  const auto* Part=Weak.Get();if(!Check(Part&&Part->GetStaticMesh()&&Part->IsVisible()&&!Part->bHiddenInGame&&Part->bRenderInMainPass&&Part->GetNumMaterials()>0,TEXT("actual static body part visible in main pass with material binding")))return false;
  Meshes.Add(Part->GetStaticMesh()->GetName());
  auto Row=S47J::New();S47J::Set(Row,TEXT("component"),Part->GetPathName());S47J::Set(Row,TEXT("mesh"),Part->GetStaticMesh()->GetPathName());
  TArray<FString> Materials;
  for(int32 Index=0;Index<Part->GetNumMaterials();++Index)
  {
   const auto* Interface=Part->GetMaterial(Index);const UMaterial* Material=Interface?Interface->GetMaterial():nullptr;
   if(!Check(Material&&Interface->GetBlendMode()==BLEND_Opaque&&!Material->bDisableDepthTest,TEXT("actual body material is opaque and depth tested, no x-ray flag")))return false;
   Materials.Add(Interface->GetPathName());
  }
  S47J::Set(Row,TEXT("materials"),Materials);Parts.Add(MakeShared<FJsonValueObject>(Row));
 }
 const TSet<FString> ExpectedMeshes={TEXT("SM_SC02_PaleTorso"),TEXT("SM_SC02_PaleHead"),TEXT("SM_SC02_PaleUpperArm"),TEXT("SM_SC02_PaleLowerArm"),TEXT("SM_SC02_PaleThigh"),TEXT("SM_SC02_PaleShin")};
 bool SameMeshes=Meshes.Num()==ExpectedMeshes.Num();for(const FString& Name:ExpectedMeshes)SameMeshes&=Meshes.Contains(Name);
 if(!Check(SameMeshes,TEXT("all six original SC02 meshes supply the ten rendered body parts")))return false;
 auto Witness=S47J::New();S47J::Set(Witness,TEXT("method"),TEXT("Current actual torso render plus camera LOS; material/main-pass depth checks, not pixel occlusion proof"));Witness->SetArrayField(TEXT("actual_body_parts"),Parts);Sight.Add(MakeShared<FJsonValueObject>(Witness));
 RenderWitness=true;return true;
}
void FS47SC11Contract::EscapeRoute()
{
 CapturesBefore=Act()->F.Captures;
 if(Mode==TEXT("sc11long")){Path({FVector2D(-2.4,2.9)});Next(60,0);}
 else{Path({FVector2D(1.2,.3),FVector2D(4.7,1.6)});Next(50,0);}
}
void FS47SC11Contract::Shot(const FString& Name)
{
 const FString File=Root/(Name+TEXT(".png"));Pictures.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);
}

void FS47SC11Contract::Tick(double Dt)
{
 if(Done||!M||!M->Story||!M->Player||!M->World||!M->Web||!M->Inter||!M->Flow||!M->UI||!M->Spine)return;
 if(!Started)
 {
  if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
  Began=FPlatformTime::Seconds();FString UserDir,Jump;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
  UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
  if(!Check((Mode==TEXT("sc11")||Mode==TEXT("sc11long")||Mode==TEXT("sc11retry"))&&M->Flow->CaseId==0&&M->Flow->bDevRun&&Jump==TEXT("chapter4")
   &&(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
   &&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir),TEXT("isolated chapter4 fixture; Saved and Saves both beneath explicit temporary profile")))return;
  Root=FPaths::ProjectSavedDir()/TEXT("SC11Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);Started=true;
  for(const TCHAR* Flag:{TEXT("S47Ch4Drive"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract"),TEXT("S47RadioWomanContract"),TEXT("S47ProductionViews")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Finish(false,TEXT("independent runtime driver also enabled"));return;}}
  if(!Check(M->Flow->Saves.List().IsEmpty()&&M->Spine->Enabled()&&M->Story->bEveningTimeline&&M->World->Area==TEXT("saro")&&Act(),TEXT("fresh isolated new-evening SC11, actual siege available")))return;
  auto* C4=Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));
  if(!Check(C4&&C4->bActive&&C4->OnEnd&&M->Player->Walkable(1.2,.3),TEXT("declared prior SC01-SC10/chapter4 fixture and legal SARO start")))return;
  for(const FS47Doc& Doc:M->Story->Docs())OriginalDocs.Add(Doc.Id,Doc.Page);
  C4->S.Stage=TEXT("complete");C4->S.bP10=C4->S.bP11=C4->S.bP12=C4->S.bTold=true;
  UE_LOG(LogTemp,Display,TEXT("S47: SC11 FIXTURE prior SC01-SC10 supplied by chapter4 jump/OnEnd; not replayed, no return drive claim"));
  M->Player->Place(1.2,.3,0);C4->OnEnd();Next(1,.6);return;
 }
 if(FPlatformTime::Seconds()-Began>300){Finish(false,TEXT("300s SC11 timeout"));return;}
 if(M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy)return;
 if(Hold>0){Hold-=Dt;return;}
 if(!Pending.IsEmpty()&&!Restored())return;
 FS47SC11* A=Act();if(!A){Finish(false,TEXT("siege owner lost"));return;}
 if(Stage>=50&&Stage<80&&A->F.Captures!=CapturesBefore){Finish(false,TEXT("unexpected actual capture on chosen escape route"));return;}
 switch(Stage)
 {
 case 1:
  if(!Check(A->Phase()==FS47SC11::EStep::Collect&&M->Story->Clock()>=FS47SC11::Arrive&&M->Story->Clock()<FS47SC11::RemoteReady&&!A->F.Escaped&&!A->HuntLive(),TEXT("05:00 collection begins before remote records or hunt"))||!SightChecks()||!Sources(TEXT("arrival")))return;
  // Manifest chair 15 ends at x=-4.57. Keep the .27m Walker footprint clear,
  // while the actual original opsTerminal remains inside its 2.4m use range.
  Path({FVector2D(0,.3),FVector2D(-4.15,2.45)});Next(2,0);return;
 case 2:if(Walk()){if(Face(TEXT("opsTerminal")))Next(3,.35);}return;
 case 3:
  if(!Use(TEXT("opsTerminal"))||!Document(TEXT("HR_SARO_RX_SECURED"),TEXT("SARO07-860414-RX01"))||!Check(A->F.Receiver&&!A->F.Escaped&&!A->HuntLive(),TEXT("receiver original secured, escape still incomplete")))return;
  Path({FVector2D(0,.3),FVector2D(2.2,1.3)});Next(4,0);return;
 case 4:if(Walk()){if(Face(TEXT("logbook")))Next(5,.35);}return;
 case 5:
  if(!Use(TEXT("logbook"))||!Document(TEXT("HR_SARO_EVENT_STARTED"),TEXT("Departure: NOT YET ENTERED"))||!Check(A->F.Event&&!A->HuntLive(),TEXT("unfinished event sheet kept without escape certification")))return;
  Path({FVector2D(2.2,.3),FVector2D(4.35,-.4)});Next(6,0);return;
 case 6:if(Walk()){if(Face(TEXT("printer")))Next(7,.35);}return;
 case 7:
  if(!Check(M->Story->Clock()<FS47SC11::RemoteReady,TEXT("negative printer use really occurs before05:04"))||!Use(TEXT("printer"))||!Check(!A->F.Hondo&&!A->F.Site11&&!A->HuntLive()&&!M->UI->Modal(),TEXT("early printer use cannot give either remote sheet or start hunt")))return;
  Path({FVector2D(2.2,.3),FVector2D(-.8,.3),FVector2D(-4.15,2.45)});Next(8,0);return;
 case 8:if(Walk()){if(Face(TEXT("opsTerminal")))Next(9,.35);}return;
 case 9:
  if(!Check(A->LocalLamp&&A->LocalLamp.Intensity()>.0001&&!A->LocalLightOff(),TEXT("actual local task light is lit before the ordinary switch use")))return;
  Snapshot(TEXT("local-task-light-before-switch"));
  if(!Use(TEXT("opsTerminal"))||!Check(A->LocalLightOff()&&A->LocalLamp.Intensity()<=.0001&&A->OutsideLightStillOn()&&A->Beams.Num()==3&&A->F.CheckedLocalLight&&!A->F.ObservedRoom&&!A->HuntLive(),TEXT("actual local lamp zero, all three external spotlights persist independently")))return;
  Snapshot(TEXT("local-off-before-observation"));Shot(TEXT("local-task-light-off"));Next(10,.5);return;
 case 10:if(Restore(TEXT("pending-local-light-off")))Next(11,.4);return;
 case 11:
  if(!Check(A->LocalLightOff()&&A->OutsideLightStillOn()&&!A->F.ObservedRoom&&!A->F.Hondo&&!A->F.Site11&&!A->F.Escaped,TEXT("pending source restore retains real off-lamp, no invented observation/remote/escape")))return;
  Path({FVector2D(1.2,.3)});Next(12,0);return;
 case 12:if(Walk()){FacePoint(M->Web->Vec(TEXT("room.windowLights.wins[1].c")));Next(13,.35);}return;
 case 13:
  if(!A->F.ObservedRoom){if(FPlatformTime::Seconds()-Mark>10)Finish(false,TEXT("actual window/camera/opaque LOS never produces room observation"));return;}
  if(!Check(A->LocalLightOff()&&A->OutsideLightStillOn()&&A->ObserveT>=.8&&S47Facing(FS47Cam::Of(M->Player),M->Web->Vec(TEXT("room.windowLights.wins[1].c")),12,35)
   &&A->ClearSight(FS47Cam::Of(M->Player).Pos,M->Web->Vec(TEXT("room.windowLights.wins[1].c")))&&M->Story->Clock()>=FS47SC11::RemoteReady&&!A->HuntLive(),TEXT("ordinary camera/LOS and actual observation dwell complete with source off and advance to05:04")))return;
  Snapshot(TEXT("actual-room-observed"));Path({FVector2D(4.35,-.4)});Next(14,0);return;
 case 14:if(Walk()){if(Face(TEXT("printer")))Next(15,.35);}return;
 case 15:
  if(!Use(TEXT("printer"))||!Document(TEXT("HR_HONDO_REMOTE"),TEXT("04:56:55"))||!Check(A->F.Hondo&&!A->F.Site11&&!A->HuntLive(),TEXT("Hondo alone cannot start hunt; Site11 remains required")))return;
  if(Restore(TEXT("hondo-not-site11")))Next(16,.4);return;
 case 16:Path({FVector2D(1.2,.3),FVector2D(-2.4,2.9)});Next(17,0);return;
 case 17:if(Walk()){if(Face(TEXT("doorSouth")))Next(18,.35);}return;
 case 18:
  if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("doorSouth")),TEXT("ordinary south door opens before the meeting; no collider bypass")))return;
  Path({FVector2D(1.2,.3),FVector2D(4.35,-.4)});Next(19,0);return;
 case 19:if(Walk()){if(Face(TEXT("printer")))Next(20,.35);}return;
 case 20:
  if(!Use(TEXT("printer"))||!Document(TEXT("HR_SITE11_REMOTE"),TEXT("04:56:56"))||!Check(A->SourcesSecured()&&A->F.CheckedLocalLight&&A->F.ObservedRoom&&!A->F.Escaped,TEXT("all four actual records collected before hunt, no source left behind"))||!Sources(TEXT("all collected")))return;
  Snapshot(TEXT("all-records-before-hunt"));Next(21,.1);return;
 case 21:
  if(!Check(A->HuntLive()&&!A->F.Escaped,TEXT("closing the final source starts real hunt")))return;
  Path({FVector2D(1.2,.3),FVector2D(-2.4,2.9)});Next(22,0);return;
 case 22:if(Walk(true))Next(23,0);return;
 case 23:
 {
  if(A->F.Captures){Finish(false,TEXT("unexpected capture before render witness/save checkpoint"));return;}
  const auto* Torso=A->Pale.Torso.Get();if(!Torso){if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("no actual body torso"));return;}
  const FVector Target=S47Three::ToThree(Torso->Bounds.Origin);FacePoint(Target);
  const FS47Cam Cam=FS47Cam::Of(M->Player);
  if(!A->Pale.Rendered()||!A->ClearSight(Cam.Pos,Target)||!S47Facing(Cam,Target,20,8))
  {if(FPlatformTime::Seconds()-Mark>25)Finish(false,TEXT("no legal actual torso render/LOS witness through the open south door"));return;}
  if(!RenderChecks())return;Snapshot(TEXT("actual-hunt-body"));Shot(TEXT("pale-body-open-door"));Next(24,.15);return;
 }
 case 24:if(Restore(TEXT("hunt-pending-no-escape")))Next(25,.4);return;
 case 25:
  if(!Check(A->HuntLive()&&A->F.Captures==0&&!A->F.Escaped,TEXT("hunt disk restore retains all sources and no free completion")))return;
  // The open south leaf reaches z=5.66. Go around its actual end before
  // crossing the corridor, rather than asking Walker to pass through it.
  if(Mode==TEXT("sc11retry")){CaptureClock=M->Story->Clock();Path({FVector2D(-2.4,6.05),FVector2D(.4,6.05),FVector2D(.4,5.75)});Next(40,0);}else EscapeRoute();return;
 case 40:
  if(A->F.Captures>0){M->Player->TestMove.Reset();Next(42,1.4);return;}
  if(Walk())Next(41,0);return;
 case 41:
  if(A->F.Captures>0){M->Player->TestMove.Reset();Next(42,1.4);return;}
  // Stay at the legally reached office/corridor. The live body must perform Grab/Capture.
  if(FPlatformTime::Seconds()-Mark>60)Finish(false,TEXT("no actual physical capture in office/corridor"));return;
 case 42:
  if(!Check(A->F.Captures==1&&A->HuntLive()&&!A->F.Escaped&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(1.2,.3))<.15
   &&M->Story->Clock()>=CaptureClock&&M->Story->Clock()-CaptureClock<90,TEXT("actual Grab/Capture resets locally, no clock regression or false escape"))||!Sources(TEXT("actual capture originals retained")))return;
  Snapshot(TEXT("actual-capture-retry"));Shot(TEXT("capture-local-retry"));Next(43,.3);return;
 case 43:EscapeRoute();return;
 case 50:if(Walk(true)){if(Face(TEXT("doorEast")))Next(51,.35);}return;
 case 51:
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Use(TEXT("doorEast")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("actual east door permits the short exposed exit")))return;
  // The landing's south rail spans x=6.3..9.4 at z=3.35..3.45.
  // Use the existing opening east of it, west of the post at x=10.95.
  Path({FVector2D(6.9,1.6),FVector2D(8.7,1.6),FVector2D(9.95,1.6),FVector2D(9.95,4.0),FVector2D(7.2,4.0)});Next(52,0);return;
 case 52:
  if(A->Phase()==FS47SC11::EStep::Done){M->Player->TestMove.Reset();Next(80,0);return;}
  if(Walk(true)){Next(53,0);}return;
 case 53:
  if(A->Phase()==FS47SC11::EStep::Done){Next(80,0);return;}
  if(FPlatformTime::Seconds()-Mark>10)Finish(false,TEXT("short truck-pad approach did not register actual escape"));return;
 case 60:if(Walk(true)){if(Face(TEXT("doorSouth")))Next(61,.35);}return;
 case 61:
  if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;
  Path({FVector2D(-2.4,5.7)});Next(62,0);return;
 case 62:if(Walk(true)){if(Face(TEXT("doorSouth")))Next(63,.35);}return;
 case 63:
  if(M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;
  if(!Check(!M->Web->IsDoorOpen(TEXT("doorSouth")),TEXT("ordinary door closes behind player to buy time on covered route")))return;
  Path({FVector2D(-4.75,5.7)});Next(64,0);return;
 case 64:if(Walk(true)){if(Face(TEXT("exitDoor")))Next(65,.35);}return;
 case 65:
  if(!M->Web->IsDoorOpen(TEXT("exitDoor"))&&!Use(TEXT("exitDoor")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("ordinary west fire exit opens; long route remains physical")))return;
  Path({FVector2D(-7,5.7),FVector2D(-11.1,5.7),FVector2D(-11.1,0),FVector2D(-9,-7.5),FVector2D(0,-12),FVector2D(6,-12),FVector2D(8.7,-12),FVector2D(8.7,-7.5),FVector2D(8.7,1.6),FVector2D(9.95,1.6),FVector2D(9.95,4.0),FVector2D(7.2,4.0)});Next(66,0);return;
 case 66:
  if(A->Phase()==FS47SC11::EStep::Done){M->Player->TestMove.Reset();Next(80,0);return;}
  if(Walk(true))Next(67,0);return;
 case 67:
  if(A->Phase()==FS47SC11::EStep::Done){Next(80,0);return;}
  if(FPlatformTime::Seconds()-Mark>10)Finish(false,TEXT("covered perimeter/truck-pad route did not register actual escape"));return;
 case 80:
  if(!Check(A->Phase()==FS47SC11::EStep::Done&&A->F.Escaped&&M->Spine->Gate(FS47Spine::SC11)==2&&M->Story->Clock()>=5*3600+10*60,TEXT("actual world escape alone completes source/gate at05:10 or later"))||!Sources(TEXT("actual escape")))return;
  Snapshot(TEXT("actual-escape-complete"));Shot(TEXT("originals-out"));Next(81,.6);return;
 case 81:
  if(M->Story->IsCinematic()||M->ModalOpen()||!M->Flow->GameSaveBlock().IsEmpty())return;
  if(Restore(TEXT("completed-world-escape")))Next(82,.5);return;
 case 82:
  if(!Check(A->Phase()==FS47SC11::EStep::Done&&A->F.Escaped&&!A->bShown&&!A->OutsideLightStillOn(),TEXT("completed disk restore does not restart hunt or erase escape")))return;
  M->Flow->Saves=FS47SaveStore(Root/TEXT("normal-save"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);Next(83,.4);return;
 case 83:
 {
  if(!M->Flow->SaveBlock().IsEmpty())return;
  FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Manual=Reader.ContinueSave();
  const auto ManualDisk=Manual?Reader.Load(Manual->Id):nullptr;
  if(!Check(Meta&&Manual&&ManualDisk&&Meta->Id==Manual->Id&&FFileHelper::SaveStringToFile(S47J::Write(ManualDisk,true),*(Root/TEXT("manual-before-title.disk.json"))),TEXT("ordinary case1 manual save, actual independent disk read: ")+Error))return;
  ++Stores;M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),FlowSelected=M->Flow->Saves.ContinueSave();
  Expected=Selected?TitleReader.Load(Selected->Id):nullptr;
  if(!Check(M->Mode==US47Main::EMode::Title&&Expected&&Selected&&FlowSelected&&Selected->Id==FlowSelected->Id
   &&FFileHelper::SaveStringToFile(S47J::Write(Expected,true),*(Root/TEXT("normal-continue.disk.json"))),TEXT("fresh reader pins actual post-title Continue disk choice")))return;
  if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens the real disk")))return;
  Pending=TEXT("normal-title-continue");NormalContinue=true;Next(84,.6);return;
 }
 case 84:
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&A->Phase()==FS47SC11::EStep::Done&&A->F.Escaped&&A->F.Captures==(Mode==TEXT("sc11retry")?1:0),TEXT("normal Continue retains actual escape/capture facts and real case")))return;
  for(const FString& File:Pictures){S47J::FObj Receipt;if(!Check(SC11Png(File,Receipt),TEXT("complete actual PNG CRC/IEND/EOF/SHA: ")+File))return;Photos.Add(MakeShared<FJsonValueObject>(Receipt));}
  if(!Check(Stores==5&&Restores.Num()==5&&RenderWitness&&NormalContinue&&Uses>=8&&Photos.Num()==(Mode==TEXT("sc11retry")?4:3)&&Walked>(Mode==TEXT("sc11long")?80:30)&&WalkSamples>50,TEXT("all required physical route, fresh disk/title, rendered body and image steps executed")))return;
  Finish(true,TEXT("SC11 records/light/room/hunt/actual escape and full disk continuation"));return;
 default:Finish(false,TEXT("unknown SC11 contract phase"));return;
 }
}

void FS47SC11Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC11Contract> Run;
 if(Owner.Get()!=Main){Owner=Main;Run=MakeShareable(new FS47SC11Contract(Main));}
 if(Run)Run->Tick(Dt);
}
