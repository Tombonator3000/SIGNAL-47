#include "S47SC04Contract.h"
#include "S47SC04.h"
#include "S47Docs.h"
#include "S47Evening.h"
#include "S47EveningPack.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47ThreatNavigation.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
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
 FString SC04Hash(const FString& File)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return {};
  FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);return Hash.ToString();
 }
 bool SC04Same(const S47J::FObj& A,const S47J::FObj& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 FString SC04Full(FString Path){Path=FPaths::ConvertRelativePathToFull(Path);FPaths::CollapseRelativeDirectories(Path);FPaths::NormalizeFilename(Path);return Path;}

 void SC04Text(const TSharedRef<SWidget>& Widget,TArray<FString>& Out)
 {
  if(!Widget->GetVisibility().IsVisible())return;
  if(Widget->GetTypeAsString()==TEXT("STextBlock"))Out.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children=Widget->GetChildren();
  for(int32 Index=0;Children&&Index<Children->Num();++Index)SC04Text(Children->GetChildAt(Index),Out);
 }
 bool SC04Png(const FString& Path,S47J::FObj& Out)
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

const FS47SC04* FS47SC04Contract::Act() const{return M&&M->Spine?&M->Spine->Secure():nullptr;}
void FS47SC04Contract::Next(int32 Value,double Wait){Stage=Value;Hold=Wait;Mark=FPlatformTime::Seconds();}
void FS47SC04Contract::Finish(bool Good,const FString& Why)
{
 if(Done)return;Done=true;
 if(ActualInput&&(SC04Hash(InputFile)!=InputHash||SC04Hash(InputResultFile)!=InputResultHash))Good=false;
 if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 auto Out=S47J::New();S47J::Set(Out,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(Out,TEXT("mode"),Mode);
 S47J::Set(Out,TEXT("stage"),Stage);S47J::Set(Out,TEXT("reason"),Why);S47J::Set(Out,TEXT("checks"),Checks);
 S47J::Set(Out,TEXT("stores"),Stores);S47J::Set(Out,TEXT("actual_ray_uses"),Uses);S47J::Set(Out,TEXT("walked_metres"),Walked);S47J::Set(Out,TEXT("walk_samples"),WalkSamples);
 S47J::Set(Out,TEXT("normal_title_continue"),NormalContinue);S47J::Set(Out,TEXT("actual_body_render_witness"),RenderWitness);
 S47J::Set(Out,TEXT("closed_after_planned_route_negative"),DoorNegative);S47J::Set(Out,TEXT("ran_away_during_grab_negative"),GrabNegative);
 S47J::Set(Out,TEXT("final_facts"),Act()?Act()->Save():S47J::FObj());S47J::Set(Out,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(Out,TEXT("method"),TEXT("Named prior SC01-SC03/T01 fixture at02:55; normal nightService/routine state and one legal control-room placement. Then native Walker TestMove/Resolve, actual camera Pick/Update/HUD OnUse, real hunt/capture or covered/perimeter route, independent disk readers/full Flow and ordinary manual/title Continue."));
 S47J::Set(Out,TEXT("limits"),TEXT("Prior SC01-SC03/T01, return drive, and their inherited papers are explicit fixtures, not replayed. sc04 measures closed-after-plan and run-away-from-Grab negatives; sc04hide measures still/closed-door escape; sc04long measures actual west/north/east traversal. Saved hunts restart their AI locally while preserving the real saved player pose, not a half-serialized chase. Later chapter restores without routine are outside this SC04 contract and belong to the continuation contract. API input is not human input. PNG integrity/render/opaque depth flags are not visual, audio, FPS or pixel-occlusion acceptance."));
 Out->SetArrayField(TEXT("executed_checks"),Executed);Out->SetArrayField(TEXT("rays"),Rays);Out->SetArrayField(TEXT("states"),States);
 Out->SetArrayField(TEXT("restores"),Restores);Out->SetArrayField(TEXT("photos"),Photos);Out->SetArrayField(TEXT("negative_observations"),Negatives);
 S47J::Set(Out,TEXT("source_prerequisite_fixtures"),!ActualInput);S47J::Set(Out,TEXT("actual_sc03_input"),ActualInput);
 if(ActualInput)
 {
  S47J::Set(Out,TEXT("chain_id"),Chain);S47J::Set(Out,TEXT("source_sc03_checkpoint"),InputFile);S47J::Set(Out,TEXT("source_sc03_checkpoint_sha1"),InputHash);
  S47J::Set(Out,TEXT("source_sc03_result"),InputResultFile);S47J::Set(Out,TEXT("source_sc03_result_sha1"),InputResultHash);
  S47J::Set(Out,TEXT("source_sc03_result_raw"),InputResult);
  S47J::Set(Out,TEXT("method"),TEXT("Actual SC03 same-night PASS/hash-bound complete disk; normal isolated case1/title/Continue, physical service-pad approach through the ordinary east door. No prior facts, clock or player placement manufactured. Subsequent ordinary Walker/camera/HUD, physical hunt, acquired originals and independent full disk/Flow/title continuation."));
  S47J::Set(Out,TEXT("limits"),TEXT("Earlier scenario acceptance is inherited only from the exact input/result bytes. No earlier fixture is invoked here. Native opaque environment trace to actual collisionless torso bounds plus camera/current render/mainpass/depth flags is not triangle/pixel/human visual acceptance. Later SC05-SC13 remain unexecuted by this driver."));
 }
 S47J::Set(Out,TEXT("requested_picture_paths"),Pictures);
 S47J::Set(Out,TEXT("scenario"),TEXT("SC04"));
 const FString Checkpoint=Root.IsEmpty()?FString():Root/TEXT("sc04-complete.disk.json");
 S47J::Set(Out,TEXT("sc04_complete_checkpoint"),Checkpoint);
 if(Good)
 {
  TArray<uint8> Bytes;
  if(!FFileHelper::LoadFileToArray(Bytes,*Checkpoint))Good=false;
  else
  {
   FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);auto Pin=S47J::New();
   S47J::Set(Pin,TEXT("path"),Checkpoint);S47J::Set(Pin,TEXT("bytes"),Bytes.Num());S47J::Set(Pin,TEXT("sha1"),Hash.ToString());
   S47J::Set(Out,TEXT("checkpoint_file"),Pin);
  }
 }
 S47J::Set(Out,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));
 const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(Out,true),*(Root/TEXT("result.json")));
 if(Good&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract %s PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm evidence=%s"),*Mode,Checks,Stores,Uses,Walked,*Root);}
 else {UE_LOG(LogTemp,Error,TEXT("S47: spine contract %s FAIL stage=%d: %s (result written %d)"),*Mode,Stage,*Why,Written);}
 FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:1);
}
bool FS47SC04Contract::Check(bool Good,const FString& Why)
{
 ++Checks;auto Row=S47J::New();S47J::Set(Row,TEXT("check"),Checks);S47J::Set(Row,TEXT("stage"),Stage);S47J::Set(Row,TEXT("pass"),Good);S47J::Set(Row,TEXT("detail"),Why);
 Executed.Add(MakeShared<FJsonValueObject>(Row));if(!Good){Finish(false,Why);return false;}
 UE_LOG(LogTemp,Display,TEXT("S47: SC04 PASS: %s"),*Why);return true;
}
void FS47SC04Contract::Path(TArray<FVector2D> Points)
{
 Route=MoveTemp(Points);Cursor=0;LastWalk=M->Player->Pos;Mark=FPlatformTime::Seconds();
}
bool FS47SC04Contract::Walk(bool Run)
{
 auto* Player=M->Player.Get();const double Distance=FVector2D(Player->Pos.X-LastWalk.X,Player->Pos.Z-LastWalk.Z).Size();
 if(Distance>.8){Finish(false,TEXT("Walker discontinuity; no placement recovery"));return false;}
 Walked+=Distance;++WalkSamples;LastWalk=Player->Pos;
 if(!Route.IsValidIndex(Cursor)){Player->TestMove.Reset();Player->bTestRun=false;return true;}
 if(FPlatformTime::Seconds()-Mark>25){Finish(false,TEXT("native Walker stalled before ")+Route[Cursor].ToString());return false;}
 const FVector2D Delta=Route[Cursor]-FVector2D(Player->Pos.X,Player->Pos.Z);
 if(Delta.Size()<.065){Player->TestMove.Reset();++Cursor;Mark=FPlatformTime::Seconds();return Cursor==Route.Num();}
 Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Y);Player->Pitch=-.04;
 Player->TestMove=FVector2D(0,-FMath::Clamp(Delta.Size()/.35,0.,1.));Player->bTestRun=Run;return false;
}
void FS47SC04Contract::FacePoint(const FVector& Three)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;Aim=S47Three::ToUE(Three);
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE()),Delta=Three-Eye;
 M->Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);M->Player->Pitch=FMath::Atan2(Delta.Y,FVector2D(Delta.X,Delta.Z).Size());
}
bool FS47SC04Contract::Face(const FString& Id)
{
 const auto* Item=M->Inter->Get(Id);FS47Obj Object=Item?Item->Object:FS47Obj();
 if(!Object)if(const auto* WebItem=M->Web->Item(Id))Object=M->Web->ObjAt(WebItem->Node);
 const FBox Box=Object.WorldBox();if(!Check(Object&&Box.IsValid,TEXT("original interaction geometry: ")+Id))return false;
 if(!Check(!OriginalNodes.Contains(Id)||OriginalNodes[Id]==Object.I,TEXT("same original interaction node: ")+Id))return false;
 OriginalNodes.Add(Id,Object.I);FacePoint(Box.GetCenter());return true;
}
bool FS47SC04Contract::Use(const FString& Id)
{
 const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Picked=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);
 auto Row=S47J::New();S47J::Set(Row,TEXT("id"),Id);S47J::Set(Row,TEXT("picked"),Picked);S47J::Set(Row,TEXT("focused"),Focus);
 S47J::Set(Row,TEXT("eye_ue_cm"),Eye.ToString());S47J::Set(Row,TEXT("forward_ue"),Forward.ToString());S47J::Set(Row,TEXT("target_distance_m"),FVector::Dist(Eye,Aim)/100.);
 const auto* Item=M->Inter->Get(Id);const auto* Exported=M->Web->Item(Id);
 const double Range=Item&&Item->Range>0?Item->Range:Exported?Exported->Range:-1;
 S47J::Set(Row,TEXT("actual_range_m"),Range);Rays.Add(MakeShared<FJsonValueObject>(Row));
 if(!Check(Picked==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary HUD OnUse: ")+Id))return false;
 M->Player->OnUse(Picked);++Uses;return true;
}
bool FS47SC04Contract::Sources(const FString& Context,bool RememberNew)
{
 TMap<FString,FString> Current;
 for(const auto& Value:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))
 {const auto Doc=Value->AsObject();if(Doc)Current.Add(S47J::Str(Doc,TEXT("id")),S47J::Write(Doc));}
 for(const auto& Saved:Documents)if(!Check(Current.Contains(Saved.Key)&&Current[Saved.Key]==Saved.Value,Context+TEXT(" exact acquired full document payload: ")+Saved.Key))return false;
 const auto* A=Act();if(!A)return Check(false,TEXT("SC04 owner missing for source custody"));
 for(const auto& Need:TArray<TPair<FString,bool>>{{TEXT("sc04_schedule"),A->Facts().bSchedule},{TEXT("sc04_strip"),A->Facts().bStrip}})
 {
  if(!Check(Current.Contains(Need.Key)==Need.Value,Context+TEXT(" source is present iff actually collected: ")+Need.Key))return false;
  if(Need.Value&&RememberNew)Documents.Add(Need.Key,Current[Need.Key]);
 }
 return true;
}
void FS47SC04Contract::Snapshot(const FString& Name)
{
 auto Row=S47J::New();S47J::Set(Row,TEXT("name"),Name);S47J::Set(Row,TEXT("clock"),M->Story->Clock());S47J::Set(Row,TEXT("area"),M->World->Area);
 S47J::Set(Row,TEXT("player_three_m"),M->Player->Pos.ToString());S47J::Set(Row,TEXT("camera_eye_ue_cm"),M->Player->EyeUE().ToString());
 S47J::Set(Row,TEXT("camera_forward_ue"),M->Player->ForwardUE().ToString());S47J::Set(Row,TEXT("checkpoint"),M->Flow->CheckpointName());
 const auto* A=Act();if(A)
 {
  S47J::Set(Row,TEXT("facts"),A->Save());S47J::Set(Row,TEXT("body_at_three_xz"),A->Pos.ToString());S47J::Set(Row,TEXT("body_state"),FS47SC04::BodyName(A->B));
  S47J::Set(Row,TEXT("body_shown"),A->bShown);S47J::Set(Row,TEXT("actual_torso_recent_render"),A->Pale.Rendered());S47J::Set(Row,TEXT("body_state_elapsed"),A->StateT);
  TArray<double> Plan;for(int32 Node:A->Path)Plan.Add(Node);S47J::Set(Row,TEXT("planned_route_nodes"),Plan);
  S47J::Set(Row,TEXT("records_door_open"),M->Web->IsDoorOpen(TEXT("recordsDoor")));S47J::Set(Row,TEXT("south_door_open"),M->Web->IsDoorOpen(TEXT("doorSouth")));
 }
 States.Add(MakeShared<FJsonValueObject>(Row));
}
bool FS47SC04Contract::Restore(const FString& Name)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;
 if(M->Story->IsCinematic()||M->World->bBusy||M->Flow->bStarting||M->ModalOpen()||!M->Flow->GameSaveBlock().IsEmpty())
 {if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("ordinary gameplay save gate still blocked: ")+Name+TEXT(" / ")+M->Flow->GameSaveBlock());return false;}
 Expected=S47J::Clone(M->Flow->GameState());
 if(!Check(S47J::Str(Expected,TEXT("checkpoint"))==TEXT("nightService")&&S47J::Obj(Expected,TEXT("routine"))&&S47J::Obj(Expected,TEXT("pose")),TEXT("normal nightService schema actually stores clock/docs/player pose: ")+Name))return false;
 FS47SaveStore Writer(Root/TEXT("cohorts")/Name);Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("SC04");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(true);
 FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,Expected,Card,Error);
 if(!Check(Meta.IsSet(),TEXT("isolated complete disk write: ")+Name+TEXT(" / ")+Error))return false;
 FS47SaveStore Reader(Writer.Dir);Reader.Init();const auto Disk=Reader.Load(Meta->Id);
 if(!Check(Disk&&S47J::Write(Disk)==S47J::Write(Expected),TEXT("independent fresh disk reader: ")+Name))return false;
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("retained full raw disk record: ")+Name))return false;
 ++Stores;Pending=Name;US47Flow::FStart Start;Start.State=Disk;Start.CaseId=ActualInput?1:0;Start.bDev=!ActualInput;Start.Playtime=M->Playtime;M->Flow->StartGame(Start);return true;
}
bool FS47SC04Contract::Restored()
{
 const auto* A=Act();const auto SavedSpine=S47J::Obj(Expected,TEXT("spine")),Saved=S47J::Obj(SavedSpine,TEXT("sc04d"));
 const auto Routine=S47J::Obj(Expected,TEXT("routine")),Pose=S47J::Obj(Expected,TEXT("pose"));
 if(!Check(A&&Saved&&Routine&&Pose&&M->Evening->NightSequence()&&M->Flow->CheckpointName()==TEXT("nightService")&&S47J::Write(A->Save())==S47J::Write(Saved)
  &&M->Spine->Gate(FS47Spine::SC04)==S47J::Int(SavedSpine,TEXT("sc04")),TEXT("full Flow retains actual facts and normal nightService gate: ")+Pending))return false;
 if(!Check(M->Story->Clock()>=S47J::Num(Routine,TEXT("clock"))&&M->Story->Clock()-S47J::Num(Routine,TEXT("clock"))<4
  &&M->World->Area==S47J::Str(Expected,TEXT("area"))&&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.04&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.04
  &&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.001
  &&S47J::Write(S47J::Obj(M->Flow->GameState(),TEXT("doors")))==S47J::Write(S47J::Obj(Expected,TEXT("doors"))),TEXT("disk restores actual clock/area/player/camera/doors without time regression: ")+Pending))return false;
 if(A->HuntLive()&&!Check(!A->F.bEscaped&&A->T<4&&A->B==FS47SC04::EBody::Away,TEXT("saved hunt restarts AI locally, actual player pose retained and no free escape")))return false;
 if(!Sources(TEXT("restored ")+Pending))return false;
 for(const auto& Node:OriginalNodes)
 {
  const auto* Interaction=M->Inter->Get(Node.Key);const auto* WebItem=M->Web->Item(Node.Key);
  const int32 Actual=Interaction&&Interaction->Object?Interaction->Object.I:WebItem?WebItem->Node:-1;
  if(!Check(Actual==Node.Value,TEXT("same original physical interaction after restore: ")+Node.Key))return false;
 }
 auto Row=S47J::New();S47J::Set(Row,TEXT("name"),Pending);S47J::Set(Row,TEXT("pass"),true);S47J::Set(Row,TEXT("facts"),A->Save());S47J::Set(Row,TEXT("clock"),M->Story->Clock());
 Restores.Add(MakeShared<FJsonValueObject>(Row));Pending.Reset();LastWalk=M->Player->Pos;return true;
}
bool FS47SC04Contract::BodySight(FHitResult& Hit,TArray<FString>& Ignored) const
{
 const auto* A=Act();if(!A||!A->Pale.Torso.IsValid())return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC04ContractOpaqueBody),true);
 for(int32 I=0;I<32;++I)
 {
  if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,M->Player->EyeUE(),A->Pale.Torso->Bounds.Origin,ECC_Visibility,Q))return true;
  const UPrimitiveComponent* C=Hit.GetComponent();if(!C)return false;
  if(A->Pale.Owns(C))return true;
  bool Solid=C->IsVisible()&&!C->bHiddenInGame;
  const auto* Node=M->Web->Node(M->Web->NodeOf(C));if(Node&&Node->Name.StartsWith(TEXT("proxy")))Solid=false;
  if(const auto* Mat=C->GetMaterial(FMath::Max<int32>(0,Hit.ElementIndex)))
  {
   float Opacity=1;Mat->GetScalarParameterValue(TEXT("Opacity"),Opacity);
   if(Mat->GetBlendMode()==BLEND_Additive||(Mat->GetBlendMode()==BLEND_Translucent&&Opacity<.5f))Solid=false;
  }
  if(Solid)return false;
  Ignored.Add(C->GetPathName());Q.AddIgnoredComponent(C);
 }
 return false;
}
bool FS47SC04Contract::WaitForBody(const FString& Context)
{
 const auto* A=Act();const auto* Torso=A?A->Pale.Torso.Get():nullptr;
 if(!Torso)return false;
 const FVector Eye=M->Player->EyeUE(),Target=Torso->Bounds.Origin;FHitResult Hit;
 TArray<FString> Ignored;const bool Block=BodySight(Hit,Ignored);
 const double Dot=FVector::DotProduct(M->Player->ForwardUE(),(Target-Eye).GetSafeNormal());
 const bool Ready=A->Pale.Rendered()&&Block&&Dot>.995;
 if(Ready)return true;
 if(FPlatformTime::Seconds()-VisibilityAt>.25)
 {
  VisibilityAt=FPlatformTime::Seconds();auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("actual-body-visibility-wait"));
  S47J::Set(Row,TEXT("body_xz"),A->Pos.ToString());S47J::Set(Row,TEXT("camera_dot"),Dot);S47J::Set(Row,TEXT("rendered"),A->Pale.Rendered());
  S47J::Set(Row,TEXT("first_opaque_component"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():TEXT("none"));S47J::Set(Row,TEXT("ignored_nonvisual_components"),Ignored);
  Negatives.Add(MakeShared<FJsonValueObject>(Row));
 }
 FacePoint(S47Three::ToThree(Target));
 if(FPlatformTime::Seconds()-Mark>35)
 {
  auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("body-visibility-timeout"));S47J::Set(Row,TEXT("context"),Context);
  S47J::Set(Row,TEXT("body_xz"),A->Pos.ToString());S47J::Set(Row,TEXT("rendered"),A->Pale.Rendered());S47J::Set(Row,TEXT("camera_dot"),Dot);
  S47J::Set(Row,TEXT("blocking_hit"),Block);S47J::Set(Row,TEXT("first_component"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():TEXT("none"));
  Negatives.Add(MakeShared<FJsonValueObject>(Row));Finish(false,TEXT("no actual unobscured body/camera witness: ")+Context);
 }
 return false;
}
bool FS47SC04Contract::RenderChecks()
{
 const auto* A=Act();if(!Check(A&&A->Pale.bBuilt&&A->Pale.All.Num()==10&&A->Pale.Rendered(),TEXT("actual ten-part pale body rendered from this legal player camera")))return false;
 const FVector Eye=M->Player->EyeUE(),Target=A->Pale.Torso->Bounds.Origin;FHitResult Hit;
 TArray<FString> Ignored;const bool Block=BodySight(Hit,Ignored);
 if(!Check(Block&&FVector::DotProduct(M->Player->ForwardUE(),(Target-Eye).GetSafeNormal())>.97,TEXT("actual camera faces native torso bounds; no visible opaque scene surface blocks the sight line")))return false;
 TSet<FString> Meshes;TArray<TSharedPtr<FJsonValue>> Parts;
 for(const auto& Weak:A->Pale.All)
 {
  const auto* Part=Weak.Get();if(!Check(Part&&Part->GetStaticMesh()&&Part->IsVisible()&&!Part->bHiddenInGame&&Part->bRenderInMainPass&&Part->GetNumMaterials()>0,TEXT("actual body part main-pass/material binding")))return false;
  Meshes.Add(Part->GetStaticMesh()->GetName());auto Row=S47J::New();S47J::Set(Row,TEXT("component"),Part->GetPathName());S47J::Set(Row,TEXT("mesh"),Part->GetStaticMesh()->GetPathName());
  TArray<FString> Materials;
  for(int32 Index=0;Index<Part->GetNumMaterials();++Index)
  {
   const auto* Interface=Part->GetMaterial(Index);const UMaterial* Material=Interface?Interface->GetMaterial():nullptr;
   if(!Check(Material&&Interface->GetBlendMode()==BLEND_Opaque&&!Material->bDisableDepthTest,TEXT("actual opaque depth-tested body material; no x-ray flag")))return false;
   Materials.Add(Interface->GetPathName());
  }
  S47J::Set(Row,TEXT("materials"),Materials);Parts.Add(MakeShared<FJsonValueObject>(Row));
 }
 const TSet<FString> Wanted={TEXT("SM_SC02_PaleTorso"),TEXT("SM_SC02_PaleHead"),TEXT("SM_SC02_PaleUpperArm"),TEXT("SM_SC02_PaleLowerArm"),TEXT("SM_SC02_PaleThigh"),TEXT("SM_SC02_PaleShin")};
 bool Same=Meshes.Num()==Wanted.Num();for(const auto& Name:Wanted)Same&=Meshes.Contains(Name);
 if(!Check(Same,TEXT("six original SC02 meshes supply all ten visible parts")))return false;
 auto Row=S47J::New();S47J::Set(Row,TEXT("scope"),TEXT("Actual opaque scene trace is clear to native torso bounds; original body meshes have no physics collision. Current torso render + mainpass/depth flags. No triangle-hit, pixel-occlusion or visual acceptance."));Row->SetArrayField(TEXT("parts"),Parts);Negatives.Add(MakeShared<FJsonValueObject>(Row));RenderWitness=true;return true;
}
bool FS47SC04Contract::Input()
{
 InputFile=SC04Full(InputFile);InputResultFile=FPaths::GetPath(InputFile)/TEXT("result.json");FString Raw,ResultRaw;
 if(!Check(FPaths::GetCleanFilename(InputFile)==TEXT("sc03-complete.disk.json")&&FFileHelper::LoadFileToString(Raw,*InputFile)&&FFileHelper::LoadFileToString(ResultRaw,*InputResultFile),TEXT("actual SC03 complete disk and sibling result readable")))return false;
 InputState=S47J::Read(Raw);InputResult=S47J::Read(ResultRaw);InputHash=SC04Hash(InputFile);InputResultHash=SC04Hash(InputResultFile);Chain=S47J::Str(InputResult,TEXT("chain_id"));
 const auto Pin=S47J::Obj(InputResult,TEXT("checkpoint_file")),Routine=S47J::Obj(InputState,TEXT("routine")),Spine=S47J::Obj(InputState,TEXT("spine")),Facts=S47J::Obj(Spine,TEXT("sc04d"));
 if(!Check(InputState&&InputResult&&S47J::Str(InputResult,TEXT("status"))==TEXT("PASS")&&S47J::Str(InputResult,TEXT("scenario"))==TEXT("SC03")&&S47J::Str(InputResult,TEXT("mode"))==TEXT("night")&&!Chain.IsEmpty()
  &&S47J::Bool(InputResult,TEXT("normal_title_continue"))&&!S47J::Bool(InputResult,TEXT("source_prerequisite_fixtures"))
  &&Pin&&SC04Full(S47J::Str(Pin,TEXT("path")))==InputFile&&S47J::Num(Pin,TEXT("bytes"),-1)==IFileManager::Get().FileSize(*InputFile)&&S47J::Str(Pin,TEXT("sha1")).Equals(InputHash,ESearchCase::IgnoreCase),TEXT("same-night SC03 PASS binds actual checkpoint path/bytes/hash, real title continuation and chain")))return false;
 if(!Check(S47J::Int(InputState,TEXT("timelineVersion"))==2&&S47J::Str(InputState,TEXT("checkpoint"))==TEXT("nightService")&&S47J::Str(InputState,TEXT("area"))==TEXT("saro")
  &&(S47J::Int(Spine,TEXT("v"))==1||S47J::Int(Spine,TEXT("v"))==2)&&S47J::Int(Spine,TEXT("sc04"))==1&&Facts&&(S47J::Int(Facts,TEXT("step"))==0||S47J::Int(Facts,TEXT("step"))==1)
  &&!S47J::Bool(Facts,TEXT("schedule"))&&!S47J::Bool(Facts,TEXT("strip"))&&!S47J::Bool(Facts,TEXT("journal"))&&!S47J::Bool(Facts,TEXT("binder"))&&!S47J::Bool(Facts,TEXT("escaped"))
  &&Routine&&S47J::Str(Routine,TEXT("phase"))==TEXT("residual")&&S47J::Int(Routine,TEXT("rxStage"))==2&&S47J::Bool(Routine,TEXT("residualVisible"))&&S47J::Bool(Routine,TEXT("powered"))&&!S47J::Bool(Routine,TEXT("k3")),TEXT("actual returned SC03 nightService cohort has unplayed pending SC04 and passive unsolved residual")))return false;
 bool Binder=false;
 for(const auto& V:S47J::Arr(Routine,TEXT("docs")))if(V&&V->Type==EJson::Object)
 {
  const auto D=V->AsObject();const FString Id=S47J::Str(D,TEXT("id"));if(!Check(!Id.IsEmpty()&&!Documents.Contains(Id),TEXT("unique actual SC03 original: ")+Id))return false;
  Documents.Add(Id,S47J::Write(D));Binder|=Id==TEXT("obs_runbinder")&&!S47J::Str(D,TEXT("page")).IsEmpty();
 }
 if(!Check(Binder,TEXT("actual20:30 run binder is already acquired by this night; never fabricated here")))return false;
 const auto Copy=[&](const FString& From,const FString& To){TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*From)&&FFileHelper::SaveArrayToFile(Bytes,*To)&&SC04Hash(From)==SC04Hash(To);};
 if(!Check(Copy(InputFile,Root/TEXT("input-sc03.byteoriginal.disk.json"))&&Copy(InputResultFile,Root/TEXT("input-sc03.byteoriginal.result.json")),TEXT("retain byteidentical actual SC03 complete source/result")))return false;
 FS47SaveCard Card;Card.Chapter=TEXT("SC03 complete actual night");Card.Place=TEXT("SARO service pad");Card.Clock=S47Text::ClockText(S47J::Num(Routine,TEXT("clock")),true);FString Error;
 const auto Meta=M->Flow->Saves.Save(1,TEXT("manual"),0,InputState,Card,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();
 if(!Check(Meta&&SC04Same(Reader.Load(Meta->Id),InputState),TEXT("isolated real case1 stores only actual SC03 full disk; independent reader: ")+Error))return false;
 ++Stores;Expected=S47J::Clone(InputState);M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave();
 if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&Selected->Id==Meta->Id&&SC04Same(TitleReader.Load(Selected->Id),InputState)&&M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title and fresh Continue load actual SC03 state")))return false;
 Pending=TEXT("actual-sc03-input");NormalContinue=true;return true;
}
bool FS47SC04Contract::Approach(const FString& Id)
{
 const auto* I=M->Inter->Get(Id);const auto* E=M->Web->Item(Id);const FS47Obj Object=I&&I->Object?I->Object:E?M->Web->ObjAt(E->Node):FS47Obj();
 const FBox Bounds=Object.WorldBox();if(!Check(Object&&Bounds.IsValid,TEXT("actual original geometry for native approach: ")+Id))return false;
 const FVector Center=Bounds.GetCenter();const FVector2D Start(M->Player->Pos.X,M->Player->Pos.Z);const double Range=I&&I->Range>0?I->Range:E&&E->Range>0?E->Range:2.4;
 const auto Traversable=[&](const FVector2D& A,const FVector2D& B)
 {
  const int32 Count=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,B)/.04));for(int32 J=0;J<=Count;++J){const FVector2D P=FMath::Lerp(A,B,double(J)/Count);if(P.X < -13 || P.X > 13 || P.Y < -13 || P.Y > 12 || !M->Player->Walkable(P.X,P.Y))return false;}return true;
 };
 const TArray<FVector2D> Seeds={{9.95,4.0},{9.95,1.6},{8.7,1.6},{7.6,1.6},{6.9,1.6},{4.7,1.6},{1.2,.3},{-2.4,2.9},{-4.15,2.45}};
 TArray<FVector2D> Best;double BestCost=1e100;
 const auto Try=[&](const FVector2D& P)
 {
  if(!Traversable(P,P))return;const FVector Eye(P.X,M->Player->FloorY+M->Player->Eye,P.Y),D=Center-Eye;
  if(D.Size()>Range-.08||M->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)return;
  auto Found=FS47ThreatNavigation::FindPath(Start,P,Seeds,M->Web->Colliders(),M->Player->Radius,Traversable);if(Found.IsEmpty())return;
  double Cost=0;FVector2D At=Start;for(const auto& Point:Found){Cost+=FVector2D::Distance(At,Point);At=Point;}if(Cost<BestCost){BestCost=Cost;Best=MoveTemp(Found);}
 };
 Try(Start);for(double Radius:{.7,1.1,1.6,2.05})for(int32 J=0;J<16;++J){const double Angle=PI*J/8.;Try({Center.X+Radius*FMath::Cos(Angle),Center.Z+Radius*FMath::Sin(Angle)});}
 if(!Check(!Best.IsEmpty(),TEXT("complete native-collider/radius/walkable route to actual item ray: ")+Id))return false;
 Path(MoveTemp(Best));return true;
}
bool FS47SC04Contract::Fixture()
{
 // Explicit inherited-night fixture. These prior episodes are not claimed as played by this contract.
 FS47EveningPack Pack;FString Error;if(!Check(Pack.Initialize(Error),TEXT("load authored evening schema for named prior-night fixture: ")+Error))return false;
 auto PackState=Pack.Save();S47J::Set(PackState,TEXT("phase"),TEXT("early_shift"));const auto Flags=S47J::Obj(PackState,TEXT("flags"));
 for(const TCHAR* Flag:{TEXT("intro_arrived"),TEXT("intro_completed"),TEXT("handover_done"),TEXT("trafo_normal_checked")})
 {if(!Check(S47J::Has(Flags,Flag),FString(TEXT("authored prior-night fixture flag exists: "))+Flag))return false;S47J::Set(Flags,Flag,true);}
 auto Evening=S47J::New();S47J::Set(Evening,TEXT("pack"),PackState);S47J::Set(Evening,TEXT("serviceRouteVersion"),2);
 for(const TCHAR* Key:{TEXT("feederFailed"),TEXT("feederRestored"),TEXT("feederCallAnswered"),TEXT("serviceDriveSkipped"),TEXT("serviceFiled")})S47J::Set(Evening,Key,true);
 S47J::Set(Evening,TEXT("clock"),2*3600.+55*60);S47J::Set(Evening,TEXT("area"),TEXT("saro"));
 M->Evening->Begin(Evening,true);
 auto Trafo=S47J::New();S47J::Set(Trafo,TEXT("v"),1);S47J::Set(Trafo,TEXT("journal"),true);S47J::Set(Trafo,TEXT("complete"),true);S47J::Set(Trafo,TEXT("observed"),true);
 S47J::Set(Trafo,TEXT("completedAt"),1*3600.+52*60);S47J::Set(Trafo,TEXT("observedAt"),1*3600.+52*60+2);S47J::Set(Trafo,TEXT("completedStamp"),TEXT("01:52 / DECLARED PRIOR T01 FIXTURE"));M->Substation->Begin(Trafo);
 auto Routine=M->Story->SaveRoutine();S47J::Set(Routine,TEXT("clock"),2*3600.+55*60);S47J::Set(Routine,TEXT("phase"),TEXT("residual"));S47J::Set(Routine,TEXT("residualVisible"),true);
 S47J::Set(Routine,TEXT("powered"),true);S47J::Set(Routine,TEXT("k3"),false);M->Story->RestoreRoutine(Routine);M->Evening->ValidateServiceRestore();
 M->Spine->Begin(nullptr,true);M->Spine->TestOpenSC04();
 if(!Check(M->Player->Walkable(-4.15,2.45),TEXT("single declared control-room start is walkable")))return false;
 M->Player->Place(-4.15,2.45,0);M->Player->Pitch=-.04;
 if(!Check(M->Evening->NightSequence()&&!M->Evening->FeederOutage()&&M->Substation->HasNormalResult()&&M->Flow->CheckpointName()==TEXT("nightService"),TEXT("declared prior SC01-SC03/T01 nightService fixture, not lossy legacy residual")))return false;
 for(const auto& Value:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs"))){const auto Doc=Value->AsObject();if(Doc)Documents.Add(S47J::Str(Doc,TEXT("id")),S47J::Write(Doc));}
 auto Record=M->Flow->GameState();S47J::Set(Record,TEXT("fixture_scope"),TEXT("SC01-SC03/T01 and return already complete by explicit test setup, not replayed. First player position is the only placement. Later custody compares these inherited papers byte-for-byte."));
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Record,true),*(Root/TEXT("prior-night.fixture.json"))),TEXT("retain exact prior-night fixture and inherited sources")))return false;
 UE_LOG(LogTemp,Display,TEXT("S47: SC04 FIXTURE explicit prior SC01-SC03/T01 normal nightService at02:55; no prior-episode/return-drive claim"));return true;
}
void FS47SC04Contract::Shot(const FString& Name)
{
 const FString File=Root/(Name+TEXT(".png"));Pictures.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);
}
void FS47SC04Contract::Tick(double Dt)
{
 if(Done||!M||!M->Story||!M->Player||!M->World||!M->Web||!M->Inter||!M->Flow||!M->UI||!M->Spine||!M->Evening||!M->Substation)return;
 if(!Started)
 {
  FParse::Value(FCommandLine::Get(),TEXT("S47SC03Checkpoint="),InputFile);ActualInput=!InputFile.IsEmpty();
  if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy||(!ActualInput&&M->Mode!=US47Main::EMode::Play))return;
  Began=FPlatformTime::Seconds();FString UserDir,Jump;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
  UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
  if(!Check((Mode==TEXT("sc04")||Mode==TEXT("sc04hide")||Mode==TEXT("sc04long"))&&(ActualInput?Jump.IsEmpty():Jump==TEXT("residual"))&&M->Flow->CaseId==0&&M->Flow->bDevRun
   &&(ActualInput||FParse::Param(FCommandLine::Get(),TEXT("S47EveningTimeline")))&&(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))
   &&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir),TEXT("isolated explicit temporary profile; Saved/Saves contained before any proof/save writes")))return;
  Root=FPaths::ProjectSavedDir()/TEXT("SC04Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);Started=true;
  for(const TCHAR* Flag:{TEXT("S47NightSequenceContract"),TEXT("S47HorrorContract"),TEXT("S47SubstationTest"),TEXT("S47SubstationTestLoad"),TEXT("S47ProductionViews"),TEXT("S47MotelV2Route"),TEXT("S47Ch4Drive"),TEXT("S47SC09Contract"),TEXT("S47RadioWomanContract")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Finish(false,TEXT("independent runtime driver also enabled"));return;}}
  if(!Check(M->Flow->Saves.List().IsEmpty()&&(ActualInput||(M->Spine->Enabled()&&M->Story->bEveningTimeline&&M->World->Area==TEXT("saro"))),TEXT("fresh isolated entry; actual SC03 consumer has no chapter jump or prior-fact setup")))return;
  if(ActualInput){if(Input())Next(601,.5);return;}
  if(!Fixture())return;Next(1,.5);return;
 }
 if(FPlatformTime::Seconds()-Began>360){Finish(false,TEXT("360s SC04 timeout"));return;}
 if(M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy)return;
 if(Hold>0){Hold-=Dt;return;}
 if(!Pending.IsEmpty()&&!Restored())return;
 const auto* A=Act();if(!A){Finish(false,TEXT("SC04 owner lost"));return;}
 if(A->bBodyUnavailable){Finish(false,TEXT("required pale body unavailable; no fallback escape accepted"));return;}
 const bool Short=Mode==TEXT("sc04"),Hide=Mode==TEXT("sc04hide"),Long=Mode==TEXT("sc04long");
 if(Long&&Stage>=60&&Stage<80&&A->F.Captures!=0){Finish(false,TEXT("unexpected capture on actual perimeter route"));return;}
 if(Stage>=20&&Stage<60&&Stage!=40&&Stage!=41&&Stage!=42&&Stage!=43&&Stage!=44&&A->Facts().Captures!=CaptureBefore)
 {Finish(false,TEXT("unexpected actual capture outside the explicit capture cohort"));return;}
 switch(Stage)
 {
 case 601:
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->Horror&&M->Horror->SC03&&M->Horror->SC03->Complete()&&M->Substation->HasNormalResult()&&!M->Evening->FeederOutage(),TEXT("normal actual SC03/T01 case restored without prerequisite fixture")))return;
  if(Approach(TEXT("doorEast")))Next(602,0);return;
 case 602:if(Walk()){if(Face(TEXT("doorEast")))Next(603,.2);}return;
 case 603:
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Use(TEXT("doorEast")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("ordinary original service door admits the actual returning player")))return;
  if(Approach(TEXT("opsTerminal")))Next(604,0);return;
 case 604:if(Walk())Next(1,.2);return;
 case 1:
 {
  if(!Check(A->Phase()==FS47SC04::EStep::Collect&&M->Spine->Gate(FS47Spine::SC04)==1&&M->Spine->HoldsResidual()&&M->Story->Clock()>=2*3600+55*60&&M->Flow->CheckpointName()==TEXT("nightService")&&!A->F.bEscaped,ActualInput?TEXT("actual SC03 cohort opens SC04 collection with normal nightService custody"):TEXT("SC03-end fixture opens actual SC04 collection at02:55 with normal nightService custody")))return;
  const int32 RxStage=M->Story->Rx.Stage;M->Story->ConsoleAction();
  if(!Check(M->Story->Rx.Stage==RxStage&&A->Label(TEXT("recPhone")).IsEmpty(),TEXT("residual console held and lure unavailable before meeting"))||!Sources(TEXT("arrival")))return;
  if(Face(TEXT("opsTerminal")))Next(2,.25);return;
 }
 case 2:
  if(!Use(TEXT("opsTerminal"))||!Check(A->F.bSchedule&&!A->F.bBinder,TEXT("ordinary ops terminal holds schedule without starting hunt"))||!Sources(TEXT("schedule"),true))return;
  Path({FVector2D(0,.3),FVector2D(4.35,-.4)});Next(3,0);return;
 case 3:if(Walk()){if(Face(TEXT("printer")))Next(4,.25);}return;
 case 4:
 {
  if(!Use(TEXT("printer")))return;
  const auto* Doc=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc04_strip");});
  TArray<FString> Text;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)SC04Text(Modal.ToSharedRef(),Text);
  if(!Check(Doc&&A->F.bStrip&&M->UI->Modal()&&Text.ContainsByPredicate([](const FString& S){return S.Contains(TEXT("Tonight's strip"))||S.Contains(TEXT("RECEIVER STRIP"));}),TEXT("actual strip source is collected and its ordinary modal is visible"))||!Sources(TEXT("strip"),true))return;
  Shot(TEXT("collected-strip-modal"));Next(105,.25);return;
 }
 case 105:M->UI->Close(true);Path({FVector2D(2.2,.3),FVector2D(2.2,1.3)});Next(5,0);return;
 case 5:if(Walk()){if(Face(TEXT("logbook")))Next(6,.25);}return;
 case 6:
  if(!Use(TEXT("logbook"))||!Check(A->F.bJournal&&!A->HuntLive()&&!A->F.bBinder&&M->Substation->HasJournal(),TEXT("shift journal taken with inherited T01 original, before binder/hunt"))||!Sources(TEXT("journal")))return;
  Next(7,.2);return;
 case 7:if(Restore(TEXT("collection-pending-binder")))Next(8,.5);return;
 case 8:Path({FVector2D(1.2,.3),FVector2D(-2.4,2.9)});Next(9,0);return;
 case 9:if(Walk()){if(Face(TEXT("doorSouth")))Next(10,.25);}return;
 case 10:
  if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("doorSouth")),TEXT("ordinary south door opens for native approach")))return;
  Path({FVector2D(-2.4,6.05),FVector2D(-3.9,6.05)});Next(11,0);return;
 case 11:if(Walk()){if(Face(TEXT("recordsDoor")))Next(12,.25);}return;
 case 12:
  if(!M->Web->IsDoorOpen(TEXT("recordsDoor"))&&!Use(TEXT("recordsDoor")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("ordinary records door opens, no disabled collider or route placement")))return;
  Path({FVector2D(-3.9,7.65),FVector2D(-2.3,7.65),FVector2D(-2.3,9.45)});Next(13,0);return;
 case 13:if(Walk()){if(Face(TEXT("binder")))Next(14,.25);}return;
 case 14:
  if(!Use(TEXT("binder"))||!Check(A->F.bBinder&&A->HuntLive()&&!A->F.bEscaped&&A->F.Captures==0,TEXT("actual binder use starts live annex meeting, no escape or capture credit"))||!Sources(TEXT("all originals")))return;
  Snapshot(TEXT("all-originals-hunt-pending"));Next(15,.1);return;
 case 15:if(Restore(TEXT("hunt-pending-no-escape")))Next(16,.4);return;
 case 16:
  CaptureBefore=0;RecordsRouteSeen=false;
  if(Long){Path({FVector2D(-2.3,7.65),FVector2D(-3.9,7.65),FVector2D(-3.9,6.05),FVector2D(-4.75,5.7)});Next(60,0);}
  else{Path({FVector2D(-2.3,7.7),FVector2D(-3.9,7.7)});Next(20,0);}return;
 // Close the real door after an actual route into records has already been computed.
 case 20:if(Walk(true)){if(Face(TEXT("recordsDoor")))Next(21,.1);}return;
 case 21:
 {
  bool RecordsPlanned=false;for(int32 Node:A->Path)RecordsPlanned|=FS47SC04::Nodes().IsValidIndex(Node)&&FS47SC04::Room(FS47SC04::Nodes()[Node].At)==2;
  RecordsRouteSeen|=RecordsPlanned;const auto* Torso=A->Pale.Torso.Get();
  if(!A->bShown||!RecordsRouteSeen||A->Pos.Y>6.65||!Torso)
  {if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("no actual pre-closure planned records route"));return;}
  if(!WaitForBody(TEXT("planned records route before ordinary closure")))return;
  if(!RenderChecks())return;Snapshot(TEXT("planned-records-route-before-close"));Shot(TEXT("pale-body-open-records-door"));
  if(Face(TEXT("recordsDoor")))Next(22,.08);return;
 }
 case 22:
  if(!Check(A->Pos.Y<6.85&&M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("body is still on corridor side when ordinary door is closed after planning"))||!Use(TEXT("recordsDoor"))||!Check(!M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("actual records door now closed")))return;
  DoorAt=FPlatformTime::Seconds();Snapshot(TEXT("closed-after-planned-route"));Next(23,0);return;
 case 23:
 {
  if(!Check(!M->Web->IsDoorOpen(TEXT("recordsDoor"))&&A->Pos.Y<=6.85&&A->F.Captures==0,TEXT("actual closed door stops body on corridor side each observed step")))return;
  auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("closed-after-route"));S47J::Set(Row,TEXT("elapsed"),FPlatformTime::Seconds()-DoorAt);S47J::Set(Row,TEXT("body_xz"),A->Pos.ToString());
  S47J::Set(Row,TEXT("blocked_door"),A->BlockedDoor);S47J::Set(Row,TEXT("door_open"),M->Web->IsDoorOpen(TEXT("recordsDoor")));Negatives.Add(MakeShared<FJsonValueObject>(Row));
  if(FPlatformTime::Seconds()-DoorAt<1.05)return;
  DoorNegative=true;Shot(TEXT("closed-door-holds-body"));
  if(Hide){Next(30,.2);return;}
  if(Face(TEXT("recordsDoor")))Next(40,.2);return;
 }
 // The long covered solution: remain still; walking after the listener arrives would make noise.
 case 30:
  if(A->Body()==FS47SC04::EBody::Listen)SawListen=true;
  if(A->F.Captures!=0||(A->Body()!=FS47SC04::EBody::Away&&FS47SC04::Room(A->Pos)==2)){Finish(false,TEXT("body captured or entered records through actual closed door"));return;}
  if(A->Phase()!=FS47SC04::EStep::Calm){if(FPlatformTime::Seconds()-Mark>145)Finish(false,TEXT("still/closed-door solution never reached actual departure"));return;}
  if(!Check(A->F.bEscaped&&SawListen,TEXT("body listened and actually returned to office while player stayed still")))return;
  if(Face(TEXT("recordsDoor")))Next(54,.2);return;
 // Short solution also probes escape from a real Grab, then one real retained-source capture.
 case 40:
  if(!Use(TEXT("recordsDoor"))||!Check(M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("ordinary reopened door lets the live body approach for Grab")))return;
  GrabClock=M->Story->Clock();Next(41,0);return;
 case 41:
  if(A->F.Captures){Finish(false,TEXT("captured before the run-away-from-Grab input could be issued"));return;}
  if(A->Body()!=FS47SC04::EBody::Grab){if(FPlatformTime::Seconds()-Mark>45)Finish(false,TEXT("no actual Grab from open records doorway"));return;}
  if(!Check(A->StateT<.35&&FVector2D::Distance(A->Pos,FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<.9,TEXT("negative escape starts from actual in-reach Grab, no posed attacker")))return;
  Snapshot(TEXT("actual-grab-before-running"));GrabAt=FPlatformTime::Seconds();Path({FVector2D(-2.3,7.7),FVector2D(-2.3,9.5),FVector2D(-1.65,9.5)});Next(42,0);return;
 case 42:
 {
  if(A->F.Captures){Finish(false,TEXT("Grab captured after player ran away during hold"));return;}
  const bool Arrived=Walk(true);if(Done)return;
  const double Away=FVector2D::Distance(A->Pos,FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
  auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("ran-away-from-grab"));S47J::Set(Row,TEXT("elapsed"),FPlatformTime::Seconds()-GrabAt);
  S47J::Set(Row,TEXT("distance_m"),Away);S47J::Set(Row,TEXT("captures"),A->F.Captures);S47J::Set(Row,TEXT("body"),FS47SC04::BodyName(A->B));Negatives.Add(MakeShared<FJsonValueObject>(Row));
  if(FPlatformTime::Seconds()-GrabAt>.85&&Away>=.9&&A->B!=FS47SC04::EBody::Grab)GrabNegative=true;
  if(!Arrived)return;
  if(!Check(GrabNegative&&A->F.Captures==0,TEXT("native running left actual reach beyond hold, Grab cancelled without capture")))return;
  M->Player->TestMove.Reset();GrabClock=M->Story->Clock();Next(43,0);return;
 }
 case 43:
  if(!A->F.Captures){GrabClock=M->Story->Clock();if(FPlatformTime::Seconds()-Mark>45)Finish(false,TEXT("live body did not perform the positive capture after the negative"));return;}
  M->Player->TestMove.Reset();Next(44,1.4);return;
 case 44:
  if(!Check(A->F.Captures==1&&A->HuntLive()&&!A->F.bEscaped&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(-2.3,9.45))<.15
   &&M->Story->Clock()>=GrabClock&&M->Story->Clock()-GrabClock<4,TEXT("actual positive capture resets to binder with no time jump/regression or escape"))||!Sources(TEXT("actual capture")))return;
  CaptureBefore=1;Snapshot(TEXT("actual-local-capture-retry"));Shot(TEXT("captured-originals-retained"));Next(45,.1);return;
 case 45:if(Restore(TEXT("actual-capture-retry")))Next(46,.4);return;
 case 46:Path({FVector2D(-2.3,7.7),FVector2D(-3.9,7.7)});Next(47,0);return;
 case 47:if(Walk(true)){if(Face(TEXT("recordsDoor")))Next(48,.12);}return;
 case 48:
  if(M->Web->IsDoorOpen(TEXT("recordsDoor"))&&!Use(TEXT("recordsDoor")))return;
  if(!Check(!M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("records door closes before retry patrol reaches it")))return;
  Path({FVector2D(-2.3,7.7),FVector2D(-2.3,9.3),FVector2D(-1.75,9.3)});Next(49,0);return;
 case 49:if(Walk(true)){if(Face(TEXT("recPhone")))Next(50,.15);}return;
 case 50:
  if(A->F.Captures!=1||M->Web->IsDoorOpen(TEXT("recordsDoor"))){Finish(false,TEXT("retry failed before stationary phone lure"));return;}
  if(A->Body()!=FS47SC04::EBody::Listen){if(FPlatformTime::Seconds()-Mark>40)Finish(false,TEXT("body never listened outside the retry door"));return;}
  if(!Use(TEXT("recPhone"))||!Check(A->F.bLure,TEXT("ordinary wall phone rings the operations office from the still safe position")))return;
  Next(51,0);return;
 case 51:
  if(A->F.Captures!=1){Finish(false,TEXT("unexpected capture while ringing office"));return;}
  if(A->Body()!=FS47SC04::EBody::Lured||FVector2D::Distance(A->Pos,FS47SC04::Nodes()[0].At)>.3)
  {if(FPlatformTime::Seconds()-Mark>25)Finish(false,TEXT("live body never reaches ringing office"));return;}
  Snapshot(TEXT("actual-office-phone-lure"));Path({FVector2D(-2.3,9.3),FVector2D(-2.3,7.7),FVector2D(-3.9,7.7)});Next(52,0);return;
 case 52:if(Walk(true)){if(Face(TEXT("recordsDoor")))Next(54,.12);}return;
 case 54:
  if(!M->Web->IsDoorOpen(TEXT("recordsDoor"))&&!Use(TEXT("recordsDoor")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("recordsDoor")),TEXT("records door actually opens for physical return to control room")))return;
  Path({FVector2D(-3.9,6.05),FVector2D(-2.4,6.05),FVector2D(-2.4,2.9)});Next(55,0);return;
 case 55:if(Walk(true)){if(Face(TEXT("doorSouth")))Next(56,.15);}return;
 case 56:
  if(M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;
  if(!Check(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("both original control-room doors really closed from inside")))return;
  Next(80,.2);return;
 // The long route leaves through the west exit and returns through the east landing.
 case 60:if(Walk(true)){if(Face(TEXT("exitDoor")))Next(61,.15);}return;
 case 61:
  if(!Use(TEXT("exitDoor"))||!Check(M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("actual west fire exit opens from corridor")))return;
  Path({FVector2D(-7,5.7),FVector2D(-11.1,5.7)});Next(62,0);return;
 case 62:if(Walk(true))Next(63,0);return;
 case 63:
 {
  const auto* Torso=A->Pale.Torso.Get();if(!A->bShown||!Torso){if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("no actual body at west opening"));return;}
  if(!WaitForBody(TEXT("west opening from actual ramp endpoint")))return;
  if(!RenderChecks())return;Snapshot(TEXT("actual-body-west-exit"));Shot(TEXT("pale-body-west-exit"));
  Path({FVector2D(-11.1,0),FVector2D(-9,-7.5),FVector2D(0,-12),FVector2D(6,-12),FVector2D(8.7,-12),FVector2D(8.7,-7.5),FVector2D(8.7,1.6),FVector2D(7.6,1.6)});Next(64,0);return;
 }
 case 64:
  if(A->bShown&&FS47SC04::Room(A->Pos)==3)HeardOutside=true;
  if(A->F.Captures){Finish(false,TEXT("actual capture on west/north perimeter route"));return;}
  if(Walk(true)){if(Face(TEXT("doorEast")))Next(65,.15);}return;
 case 65:
  if(!Use(TEXT("doorEast"))||!Check(M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("actual east service door opens from landing")))return;
  Path({FVector2D(6.9,1.6),FVector2D(4.3,1.6)});Next(66,0);return;
 case 66:if(Walk(true)){if(Face(TEXT("doorEast")))Next(67,.15);}return;
 case 67:
  if(!Use(TEXT("doorEast"))||!Check(!M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("east service door really closes behind the returning player")))return;
  Path({FVector2D(1.2,.3),FVector2D(-2.4,2.9)});Next(55,0);return;
 case 80:
  if(Long&&A->bShown&&FS47SC04::Room(A->Pos)==3)HeardOutside=true;
  if(A->F.Captures!=CaptureBefore){Finish(false,TEXT("capture after reaching control-room cover"));return;}
  if(M->Spine->Gate(FS47Spine::SC04)!=2){if(FPlatformTime::Seconds()-Mark>145)Finish(false,TEXT("actual SC04 never completed behind the closed control-room doors"));return;}
  if(!Check(A->Phase()==FS47SC04::EStep::Done&&A->F.bEscaped&&!M->Spine->HoldsResidual()&&M->Story->Clock()>=3*3600+15*60&&A->F.Captures==(Short?1:0)&&A->F.bLure==Short,TEXT("real escape/locked control room alone releases residual at03:15, correct capture/lure facts"))||!Sources(TEXT("completed")))return;
  if(Long&&!Check(HeardOutside,TEXT("body actually followed west-exit sound outside during the physical perimeter route")))return;
  Snapshot(TEXT("actual-escape-complete"));Shot(TEXT("originals-in-control-room"));Next(81,.4);return;
 case 81:if(Restore(TEXT("sc04-complete")))Next(82,.5);return;
 case 82:
  if(!Check(A->Phase()==FS47SC04::EStep::Done&&A->F.bEscaped&&!A->bShown,TEXT("completed full Flow keeps actual escape and does not restart the body")))return;
  M->Flow->Saves=FS47SaveStore(Root/TEXT("normal-save"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);Next(83,.2);return;
 case 83:
 {
  if(!M->Flow->SaveBlock().IsEmpty()){if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("ordinary manual save remains blocked"));return;}
  FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Manual=Reader.ContinueSave();const auto ManualDisk=Manual?Reader.Load(Manual->Id):nullptr;
  if(!Check(Meta&&Manual&&ManualDisk&&Meta->Id==Manual->Id&&FFileHelper::SaveStringToFile(S47J::Write(ManualDisk,true),*(Root/TEXT("manual-before-title.disk.json"))),TEXT("ordinary isolated case1 manual write and independent disk reader: ")+Error))return;
  ++Stores;M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),FlowSelected=M->Flow->Saves.ContinueSave();Expected=Selected?TitleReader.Load(Selected->Id):nullptr;
  if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&FlowSelected&&Expected&&Selected->Id==FlowSelected->Id&&FFileHelper::SaveStringToFile(S47J::Write(Expected,true),*(Root/TEXT("normal-continue.disk.json"))),TEXT("fresh reader pins the actual post-title Continue choice")))return;
  if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens actual saved disk")))return;
  Pending=TEXT("normal-title-continue");NormalContinue=true;Next(84,.6);return;
 }
 case 84:
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&A->Phase()==FS47SC04::EStep::Done&&A->F.bEscaped&&A->SourcesReady(),TEXT("normal Continue keeps real case and complete acquired originals")))return;
  Snapshot(TEXT("normal-title-continue"));Shot(TEXT("after-normal-continue"));Next(85,.5);return;
 case 85:
  for(const FString& File:Pictures){S47J::FObj Receipt;if(!SC04Png(File,Receipt)&&FPlatformTime::Seconds()-Mark<8)return;}
  for(const FString& File:Pictures){S47J::FObj Receipt;if(!Check(SC04Png(File,Receipt),TEXT("actual PNG complete CRC/IEND/EOF/SHA: ")+File))return;Photos.Add(MakeShared<FJsonValueObject>(Receipt));}
  if(!Check(Stores==(Short?5:4)+(ActualInput?1:0)&&Restores.Num()==Stores&&RenderWitness&&NormalContinue&&Uses>=(Hide?9:10)&&Walked>(Long?85:35)&&WalkSamples>100
   &&Photos.Num()==(Short?6:Hide?5:4)&&(!Short||(DoorNegative&&GrabNegative))&&(!Hide||DoorNegative),TEXT("all required route/negative/body/source/disk/title/PNG steps actually executed")))return;
  Finish(true,TEXT("SC04 physical collection, live annex route/capture, source custody and ordinary disk/title continuation"));return;
 default:Finish(false,TEXT("unknown SC04 contract stage"));return;
 }
}
void FS47SC04Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC04Contract> Run;
 if(Owner.Get()!=Main){Owner=Main;Run=MakeShareable(new FS47SC04Contract(Main));}
 if(Run)Run->Tick(Dt);
}
