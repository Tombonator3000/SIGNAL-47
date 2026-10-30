#include "S47SC08Contract.h"
#include "S47SC08.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Spine.h"
#include "S47Flow.h"
#include "S47Office.h"
#include "S47Inter.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47ReelTape.h"
#include "S47SaroNight.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

namespace
{
 S47J::FObj SC08File(const FString& Path)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return nullptr;
  FSHAHash H;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),H.Hash);auto O=S47J::New();S47J::Set(O,TEXT("path"),Path);S47J::Set(O,TEXT("bytes"),Bytes.Num());S47J::Set(O,TEXT("sha1"),H.ToString());return O;
 }
 bool SC08Same(const S47J::FObj& One,const S47J::FObj& Two){return One&&Two&&S47J::Write(One)==S47J::Write(Two);}
 bool SC08Png(const FString& Path,S47J::FObj& Out)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*Path))return false;const uint8 Sig[]={137,80,78,71,13,10,26,10};
  if(B.Num()<33||FMemory::Memcmp(B.GetData(),Sig,8))return false;
  const auto U32=[&](int64 At){return(uint32(B[At])<<24)|(uint32(B[At+1])<<16)|(uint32(B[At+2])<<8)|B[At+3];};
  int64 At=8,Data=0;int32 Chunks=0;uint32 Width=0,Height=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>B.Num()-At-12)return false;const uint8* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(!Chunks){if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13)return false;Width=U32(At+8);Height=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Data+=Size;++Chunks;At+=int64(Size)+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=B.Num()||!Data||Width<320||Height<200)return false;
  Out=SC08File(Path);S47J::Set(Out,TEXT("width"),double(Width));S47J::Set(Out,TEXT("height"),double(Height));S47J::Set(Out,TEXT("crc_iend_eof"),true);return true;
 }
}
FS47SC08* FS47SC08Contract::Act() const{return M&&M->Spine?M->Spine->Array():nullptr;}
void FS47SC08Contract::Next(int32 Value,double Delay){Stage=Value;Wait=Delay;Mark=FPlatformTime::Seconds();}
bool FS47SC08Contract::Check(bool Good,const FString& Why)
{
 ++Checks;auto R=S47J::New();S47J::Set(R,TEXT("check"),Checks);S47J::Set(R,TEXT("stage"),Stage);S47J::Set(R,TEXT("pass"),Good);S47J::Set(R,TEXT("detail"),Why);Executed.Add(MakeShared<FJsonValueObject>(R));
 if(!Good){Finish(false,Why);return false;}UE_LOG(LogTemp,Display,TEXT("S47: SC08 PASS: %s"),*Why);return true;
}
void FS47SC08Contract::Finish(bool Good,const FString& Why)
{
 if(Done)return;Done=true;if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}if(M&&M->World)M->World->Autopilot.Reset();
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("scenario"),TEXT("SC08"));S47J::Set(O,TEXT("mode"),Mode);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("reason"),Why);
 S47J::Set(O,TEXT("chain_id"),ChainId);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stores"),Stores);S47J::Set(O,TEXT("actual_ray_uses"),Uses);S47J::Set(O,TEXT("walked_metres"),Walked);S47J::Set(O,TEXT("walk_samples"),WalkSamples);S47J::Set(O,TEXT("normal_title_continue"),NormalContinue);
 S47J::Set(O,TEXT("actual_body_render_witness"),BodyWitness);S47J::Set(O,TEXT("actual_opaque_footing_cover"),CoverWitness);S47J::Set(O,TEXT("actual_search_read"),SearchRead);S47J::Set(O,TEXT("ran_away_during_grab_negative"),GrabNegative);
 S47J::Set(O,TEXT("prerequisite"),PriorReceipt);S47J::Set(O,TEXT("checkpoint_file"),Root.IsEmpty()?nullptr:SC08File(Root/TEXT("sc08-complete.disk.json")));S47J::Set(O,TEXT("final_facts"),Act()?Act()->Readback():nullptr);S47J::Set(O,TEXT("final_state"),M&&M->Flow?M->Flow->GameState():nullptr);
 S47J::Set(O,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);S47J::Set(O,TEXT("driven_out_metres"),(OdoOut-OdoStart)*1609.344);S47J::Set(O,TEXT("driven_return_metres"),M&&M->World?(M->World->Odometer()-OdoOut)*1609.344:0);
 S47J::Set(O,TEXT("method"),TEXT("Actual hash-bound SC07 PASS/disk; full Flow. Native Walker/Resolve with current dynamic colliders, actual camera Pick/focus/HUD OnUse; ordinary truck trips driven by existing autopilot, ordinary parking/get-out; actual original isolated reel twice, A/B/return and native encoder instances, native-bounds/render witness and live field search/escape, full disk/readers/Flow/manual/title Continue."));
 S47J::Set(O,TEXT("limits"),TEXT("SC01-SC07 come only from the supplied actual predecessor, not replayed or fabricated. Truck steering is existing test autopilot, not human input. Recorded caller availability is measured separately from completed original cue playback; new Earl voice is text only. API buttons are the production panel callbacks. Short route reads the search; long route requires a real opaque native footing hit; retry requires actual failed-grab cancellation and capture. Body witness is physicsClearToNativeBounds plus cameraDot/recent native render and opaque depth flags; no body physics-hit, triangle-hit or pixel-occlusion claim. PNG integrity/render/depth flags are not visual/audio/FPS/full-matrix/install/release acceptance. Chapter3 starts through the original card at04:25; no SC09 action is performed."));
 O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("rays"),Rays);O->SetArrayField(TEXT("states"),States);O->SetArrayField(TEXT("restores"),Restores);O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("negative_observations"),Negatives);
 const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 if(Good&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract %s PASS COMPLETE: %d checks, %d stores, %.3fm evidence=%s"),*Mode,Checks,Stores,Walked,*Root);}
 else{UE_LOG(LogTemp,Error,TEXT("S47: spine contract %s FAIL stage=%d: %s; result written=%d evidence=%s"),*Mode,Stage,*Why,Written,*Root);}
 FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:1);
}
bool FS47SC08Contract::Clear(const FVector2D& From,const FVector2D& To) const
{
 const FVector2D D=To-From;
 for(const auto& C:M->Web->Colliders())if(C.bEnabled)
 {
  if(C.Id==TEXT("truck")&&M->Player->TruckBox&&M->Player->TruckBox->Area==M->World->Area)continue;
  double T0=0,T1=1;const auto Clip=[&](double Pn,double Q){if(FMath::Abs(Pn)<1e-12)return Q>=0;const double R=Q/Pn;if(Pn<0){if(R>T1)return false;T0=FMath::Max(T0,R);}else{if(R<T0)return false;T1=FMath::Min(T1,R);}return true;};
  if(Clip(-D.X,From.X-C.MinX+M->Player->Radius)&&Clip(D.X,C.MaxX+M->Player->Radius-From.X)&&Clip(-D.Y,From.Y-C.MinZ+M->Player->Radius)&&Clip(D.Y,C.MaxZ+M->Player->Radius-From.Y))return false;
 }
 const int32 N=FMath::Max(1,FMath::CeilToInt(D.Size()/.08));
 for(int32 I=0;I<=N;++I){const auto At=FMath::Lerp(From,To,double(I)/N);if(!M->Player->Walkable(At.X,At.Y)||M->Player->InTruck(At.X,At.Y))return false;}return true;
}
void FS47SC08Contract::Path(TArray<FVector2D> Points,int32 NextStage){Route=MoveTemp(Points);Cursor=0;LastWalk=M->Player->Pos;Next(NextStage,0);}
bool FS47SC08Contract::Plan(const FVector2D& Goal,int32 NextStage)
{
 // Small test-input planner, never a replacement for Resolve. It samples live dynamic colliders.
 const FVector2D Start(M->Player->Pos.X,M->Player->Pos.Z);if(!Check(Clear(Start,Start)&&Clear(Goal,Goal),TEXT("actual route endpoints clear of active scene colliders")))return false;
 if(Clear(Start,Goal)){Path({Goal},NextStage);return true;}
 struct FNode{FIntPoint Cell;double G,F;int32 Parent;};TArray<FNode> Nodes;TMap<FIntPoint,int32> Seen;TSet<int32> Closed;TArray<int32> Open;
 constexpr double Grid=.4;const auto Point=[&](const FIntPoint& C){return Start+FVector2D(C.X*Grid,C.Y*Grid);};
 Nodes.Add({FIntPoint(0,0),0,FVector2D::Distance(Start,Goal),-1});Seen.Add(FIntPoint(0,0),0);Open.Add(0);int32 End=-1;
 const double Bound=FVector2D::Distance(Start,Goal)+35;
 while(!Open.IsEmpty()&&Nodes.Num()<70000)
 {
  int32 Pick=0;for(int32 I=1;I<Open.Num();++I)if(Nodes[Open[I]].F<Nodes[Open[Pick]].F)Pick=I;const int32 Index=Open[Pick];Open.RemoveAtSwap(Pick);if(Closed.Contains(Index))continue;Closed.Add(Index);
  const auto Current=Nodes[Index];const FVector2D At=Point(Current.Cell);
  if(FVector2D::Distance(At,Goal)<.65&&Clear(At,Goal)){End=Index;break;}
  for(int32 Dx=-1;Dx<=1;++Dx)for(int32 Dz=-1;Dz<=1;++Dz)
  {
   if(!Dx&&!Dz)continue;const FIntPoint Cell=Current.Cell+FIntPoint(Dx,Dz);const FVector2D To=Point(Cell);if(FVector2D::Distance(Start,To)+FVector2D::Distance(To,Goal)>Bound||!Clear(At,To))continue;
   const double G=Current.G+FVector2D::Distance(At,To);const int32* Was=Seen.Find(Cell);if(Was&&Nodes[*Was].G<=G)continue;
   const int32 K=Was?*Was:Nodes.Num();if(!Was){Seen.Add(Cell,K);Nodes.Add({Cell,G,G+FVector2D::Distance(To,Goal),Index});}else{Nodes[K].G=G;Nodes[K].F=G+FVector2D::Distance(To,Goal);Nodes[K].Parent=Index;Closed.Remove(K);}Open.Add(K);
  }
 }
 if(!Check(End>=0,TEXT("native-zone/live-collider input route exists without placement recovery")))return false;
 TArray<FVector2D> Points={Goal};for(int32 I=End;I>0;I=Nodes[I].Parent)Points.Insert(Point(Nodes[I].Cell),0);Path(MoveTemp(Points),NextStage);return true;
}
bool FS47SC08Contract::Walk(bool Run)
{
 const auto At=M->Player->Pos;const double Moved=FVector2D(At.X-LastWalk.X,At.Z-LastWalk.Z).Size();LastWalk=At;
 if(Moved>.8){Finish(false,TEXT("native Walker discontinuity; no Place recovery"));return false;}Walked+=Moved;++WalkSamples;
 if(Act()&&Act()->HuntLive()){SearchRead|=Act()->Behavior==FS47SC08::EBody::Search;ObserveCover();}
 if(!Route.IsValidIndex(Cursor)){M->Player->TestMove.Reset();M->Player->bTestRun=false;return true;}
 if(FPlatformTime::Seconds()-Mark>20){Finish(false,TEXT("native Walker blocked at ")+Route[Cursor].ToString());return false;}
 const FVector2D D=Route[Cursor]-FVector2D(At.X,At.Z);if(D.Size()<.065){++Cursor;Mark=FPlatformTime::Seconds();M->Player->TestMove.Reset();return Cursor==Route.Num();}
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Y);M->Player->Pitch=-.04;M->Player->TestMove=FVector2D(0,-FMath::Clamp(D.Size()/.35,0.,1.));M->Player->bTestRun=Run;return false;
}
bool FS47SC08Contract::Approach(const FString& Id,int32 NextStage)
{
 const auto* I=M->Inter->Get(Id);FS47Obj Obj=I?I->Object:FS47Obj();if(!Obj)if(const auto* E=M->Web->Item(Id))Obj=M->Web->ObjAt(E->Node);
 const FBox Box=Obj.WorldBox();if(!Check(Obj&&Box.IsValid,TEXT("actual original target for approach: ")+Id))return false;
 const FVector C=Box.GetCenter();const FVector2D Start(M->Player->Pos.X,M->Player->Pos.Z);FVector2D Best;double Distance=1e20;
 for(double R:{1.5,1.9,2.3})for(int32 K=0;K<16;++K)
 {
  const double Angle=2*PI*K/16;const FVector2D P(C.X+R*FMath::Cos(Angle),C.Z+R*FMath::Sin(Angle));
  if(Clear(P,P)&&FVector2D::Distance(Start,P)<Distance){Best=P;Distance=FVector2D::Distance(Start,P);}
 }
 if(!Check(Distance<1e19,TEXT("walkable original target approach, no placement: ")+Id))return false;return Plan(Best,NextStage);
}
void FS47SC08Contract::FacePoint(const FVector& Three)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;Aim=S47Three::ToUE(Three);const FVector D=Three-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
bool FS47SC08Contract::Face(const FString& Id)
{
 const auto* I=M->Inter->Get(Id);FS47Obj Obj=I?I->Object:FS47Obj();if(!Obj)if(const auto* E=M->Web->Item(Id))Obj=M->Web->ObjAt(E->Node);const FBox Box=Obj.WorldBox();if(!Check(Obj&&Box.IsValid,TEXT("original physical target: ")+Id))return false;
 const auto* Node=M->Web->Node(Obj.I);const auto* Primitive=Node?Cast<UPrimitiveComponent>(Node->Comp.Get()):nullptr;
 FacePoint(Id.StartsWith(TEXT("sc08:"))&&Primitive?S47Three::ToThree(Primitive->Bounds.Origin):Box.GetCenter());return true;
}
bool FS47SC08Contract::Use(const FString& Id)
{
 const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Pick=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);const auto* I=M->Inter->Get(Id);const auto* E=M->Web->Item(Id);const double Range=I&&I->Range>0?I->Range:E?E->Range:2.4;
 auto R=S47J::New();S47J::Set(R,TEXT("id"),Id);S47J::Set(R,TEXT("picked"),Pick);S47J::Set(R,TEXT("focus"),Focus);S47J::Set(R,TEXT("eye_ue_cm"),Eye.ToString());S47J::Set(R,TEXT("forward_ue"),Forward.ToString());S47J::Set(R,TEXT("actual_range_m"),Range);Rays.Add(MakeShared<FJsonValueObject>(R));
 if(!Check(Pick==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary HUD OnUse: ")+Id))return false;M->Player->OnUse(Pick);++Uses;return true;
}
bool FS47SC08Contract::Documents(bool Remember)
{
 TMap<FString,FString> Now;for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object){const auto D=V->AsObject();Now.Add(S47J::Str(D,TEXT("id")),S47J::Write(D));}
 for(const auto& D:Papers)if(!Check(Now.Contains(D.Key)&&Now[D.Key]==D.Value,TEXT("exact acquired page/stamp/image/sheets preserved: ")+D.Key))return false;
 if(Remember)Papers=MoveTemp(Now);return true;
}
bool FS47SC08Contract::Store(const FString& Name,bool Final)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;
 if(M->ModalOpen()||M->Story->IsCinematic()||M->World->bBusy||M->Flow->bStarting||!M->Flow->GameSaveBlock().IsEmpty()){if(FPlatformTime::Seconds()-Mark>25)Finish(false,TEXT("normal save gate still blocked: ")+Name);return false;}
 if(!Documents(true))return false;Expected=S47J::Clone(M->Flow->GameState());FS47SaveStore Writer(Root/TEXT("cohorts")/Name);Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("SC08");Card.Clock=M->Story->ClockText(true);Card.Place=M->Flow->PlaceName();FString Error;
 const auto Meta=Writer.Save(1,TEXT("manual"),0,Expected,Card,Error);FS47SaveStore Reader(Writer.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):nullptr;
 if(!Check(Disk&&SC08Same(Disk,Expected)&&FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("full isolated disk + independent fresh reader: ")+Name+Error))return false;
 if(Final&&!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("sc08-complete.disk.json"))),TEXT("stable actual SC08 complete checkpoint for SC09")))return false;
 Pending=Name;++Stores;US47Flow::FStart Start;Start.State=Disk;Start.bDev=true;Start.Playtime=M->Playtime;M->Flow->StartGame(Start);return true;
}
bool FS47SC08Contract::Restored()
{
 const auto WantCase=S47J::Obj(Expected,TEXT("case")),Pose=S47J::Obj(Expected,TEXT("pose")),WantSpine=S47J::Obj(Expected,TEXT("spine")),Want=S47J::Obj(WantSpine,TEXT("sc08d"));const auto Actual=M->Flow->GameState();auto* A=Act();
 if(!Check(A&&Want&&M->Spine->Gate(FS47Spine::SC08)==S47J::Int(WantSpine,TEXT("sc08"))&&A->Phase()==FS47SC08::EStep(S47J::Int(Want,TEXT("step")))&&A->Plays==S47J::Int(Want,TEXT("plays"))&&A->Samples.Num()==S47J::Arr(Want,TEXT("samples")).Num()&&A->Captures==S47J::Int(Want,TEXT("captures"))&&A->bEscaped==S47J::Bool(Want,TEXT("escaped"))&&A->EscapedAt==S47J::Num(Want,TEXT("escapedAt")),TEXT("full Flow preserves SC08 progress/capture/actual escape only: ")+Pending))return false;
 if(!Check(SC08Same(S47J::Obj(S47J::Obj(Actual,TEXT("case")),TEXT("s")),S47J::Obj(WantCase,TEXT("s")))&&SC08Same(S47J::Obj(S47J::Obj(Actual,TEXT("case")),TEXT("ch2")),S47J::Obj(WantCase,TEXT("ch2")))&&SC08Same(S47J::Obj(Actual,TEXT("office")),S47J::Obj(Expected,TEXT("office")))&&SC08Same(S47J::Obj(Actual,TEXT("doors")),S47J::Obj(Expected,TEXT("doors"))),TEXT("SC06/SC07 actual photos/coords/Office/doors preserved: ")+Pending))return false;
 if(!Check(M->Story->Clock()>=S47J::Num(WantCase,TEXT("clock"))&&M->Story->Clock()-S47J::Num(WantCase,TEXT("clock"))<4&&M->World->Area==S47J::Str(Expected,TEXT("area"))&&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.04&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.04&&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.001,TEXT("real saved player/camera/area and forward-only clock restored: ")+Pending)||!Documents())return false;
 auto Angles=S47J::New();Angles->SetArrayField(TEXT("pointings"),S47J::Arr(Want,TEXT("pointings")));
 if(!Check(SC08Same(Angles,A->AppliedRestore)&&S47J::Arr(Want,TEXT("pointings")).Num()==27,TEXT("all 27 actual native encoder values were restored before further motion: ")+Pending))return false;
 const auto Now=A->Save();for(const TCHAR* Field:{TEXT("samples"),TEXT("sources")})
 {
  auto Left=S47J::New(),Right=S47J::New();Left->SetArrayField(Field,S47J::Arr(Now,Field));Right->SetArrayField(Field,S47J::Arr(Want,Field));
  if(!Check(SC08Same(Left,Right),FString(TEXT("exact acquired SC08 records preserved: "))+Field+TEXT(" / ")+Pending))return false;
 }
 if(A->HuntLive()&&!Check(!A->bEscaped&&A->T<4,TEXT("restored hunt restarts locally, real player pose retained, no fabricated escape")))return false;
 auto R=S47J::New();S47J::Set(R,TEXT("name"),Pending);S47J::Set(R,TEXT("pass"),true);S47J::Set(R,TEXT("actual"),A->Readback());Restores.Add(MakeShared<FJsonValueObject>(R));Pending.Reset();LastWalk=M->Player->Pos;return true;
}
void FS47SC08Contract::Snapshot(const FString& Name)
{
 auto R=S47J::New();S47J::Set(R,TEXT("name"),Name);S47J::Set(R,TEXT("clock"),M->Story->Clock());S47J::Set(R,TEXT("area"),M->World->Area);S47J::Set(R,TEXT("player_three_m"),M->Player->Pos.ToString());S47J::Set(R,TEXT("camera_eye_ue_cm"),M->Player->EyeUE().ToString());S47J::Set(R,TEXT("camera_forward_ue"),M->Player->ForwardUE().ToString());S47J::Set(R,TEXT("readback"),Act()?Act()->Readback():nullptr);States.Add(MakeShared<FJsonValueObject>(R));
}
void FS47SC08Contract::Shot(const FString& Name){const FString File=Root/(Name+TEXT(".png"));Pictures.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);}
bool FS47SC08Contract::RenderBody()
{
 auto* A=Act();if(!A||!A->Pale.bBuilt||!A->bShown||!A->Pale.Torso.IsValid())return false;
 const auto* Torso=A->Pale.Torso.Get();FacePoint(S47Three::ToThree(Torso->Bounds.Origin));
 if(!A->Pale.Rendered()||FVector::DotProduct(M->Player->ForwardUE(),(Torso->Bounds.Origin-M->Player->EyeUE()).GetSafeNormal())<.995)return false;
 // These original meshes intentionally have no physics mesh collision. No hit is legitimate;
 // a visible opaque world surface before the actual native bounds is not.
 if(!A->ClearSight(S47Three::ToThree(M->Player->EyeUE()),S47Three::ToThree(Torso->Bounds.Origin)))return false;
 if(!Check(A->Pale.All.Num()==10,TEXT("actual camera-visible ten-part body: clear opaque sight to native torso bounds + render flags; no physics/triangle hit claim")))return false;
 TSet<FString> Meshes;
 for(const auto& Weak:A->Pale.All)
 {
  const auto* Part=Weak.Get();if(!Check(Part&&Part->GetStaticMesh()&&Part->IsVisible()&&!Part->bHiddenInGame&&Part->bRenderInMainPass&&Part->GetNumMaterials()>0,TEXT("native visible body component/material/main pass")))return false;Meshes.Add(Part->GetStaticMesh()->GetName());
  for(int32 I=0;I<Part->GetNumMaterials();++I){const auto* Mat=Part->GetMaterial(I);const auto* Base=Mat?Mat->GetMaterial():nullptr;if(!Check(Base&&Mat->GetBlendMode()==BLEND_Opaque&&!Base->bDisableDepthTest,TEXT("actual body material opaque/depth-tested; no x-ray setting")))return false;}
 }
 if(!Check(Meshes.Num()==6,TEXT("the original six SC02 meshes supply all ten body parts")))return false;BodyWitness=true;return true;
}
void FS47SC08Contract::ObserveCover()
{
 if(CoverWitness||!Act()||!Act()->bShown)return;auto* A=Act();const FVector From(A->Pos.X,A->Ground(A->Pos)+1.72,A->Pos.Y),To=S47Three::ToThree(M->Player->EyeUE());
 if(FVector::Dist(From,To)>40||A->ClearSight(From,To))return;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC08FootingCover),true);for(const auto& P:A->Pale.All)if(P.IsValid())Q.AddIgnoredComponent(P.Get());
 FHitResult Hit;if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(From),S47Three::ToUE(To),ECC_Visibility,Q))return;
 const auto* C=Hit.GetComponent();const int32 Node=M->Web->NodeOf(C);const auto* Material=C?C->GetMaterial(FMath::Max<int32>(0,Hit.ElementIndex)):nullptr;
 const bool Footing=Node==M->Web->Handle(TEXT("ext.dishArray.parts[0].mesh"))||Node==M->Web->Handle(TEXT("ext.dishArray.parts[1].mesh"));
 if(!C||!C->IsVisible()||!Footing||!A->Dishes.Contains(Hit.Item)||!Material||Material->GetBlendMode()!=BLEND_Opaque)return;
 auto R=S47J::New();S47J::Set(R,TEXT("kind"),TEXT("actual opaque near-dish footing first visibility hit"));S47J::Set(R,TEXT("node"),Node);S47J::Set(R,TEXT("component"),C->GetPathName());S47J::Set(R,TEXT("instance"),Hit.Item);S47J::Set(R,TEXT("material"),Material->GetPathName());S47J::Set(R,TEXT("hit_three_m"),S47Three::ToThree(Hit.ImpactPoint).ToString());S47J::Set(R,TEXT("from_three_m"),From.ToString());S47J::Set(R,TEXT("player_eye_three_m"),To.ToString());Negatives.Add(MakeShared<FJsonValueObject>(R));CoverWitness=true;
}
void FS47SC08Contract::Tick(double Dt)
{
 if(Done||!M||!M->Story||!M->Flow||!M->World||!M->Player||!M->Inter||!M->UI||!M->Web||!M->Spine||!M->Office||!M->SaroNight)return;
 if(!Started)
 {
  if(M->Mode!=US47Main::EMode::Play||S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy)return;
  Began=FPlatformTime::Seconds();FString User,Jump;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);FParse::Value(FCommandLine::Get(),TEXT("S47SC07Checkpoint="),PriorPath);User=FPaths::ConvertRelativePathToFull(User);FPaths::CollapseRelativeDirectories(User);
  if(!Check((Mode==TEXT("sc08")||Mode==TEXT("sc08long")||Mode==TEXT("sc08retry"))&&Jump==TEXT("chapter2")&&FParse::Param(FCommandLine::Get(),TEXT("S47EveningTimeline"))&&M->Flow->CaseId==0&&M->Flow->bDevRun&&M->Flow->Saves.List().IsEmpty()
   &&(User.StartsWith(TEXT("/tmp/"))||User.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),User)&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),User),TEXT("fresh isolated developer profile; Saved/Saves contained before proof writes")))return;
  Root=FPaths::ProjectSavedDir()/TEXT("SC08Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);Started=true;
  for(const TCHAR* Flag:{TEXT("S47ProductionViews"),TEXT("S47SC09Contract"),TEXT("S47HorrorContract"),TEXT("S47NightSequenceContract"),TEXT("S47SubstationTest"),TEXT("S47Drive"),TEXT("S47Walk"),TEXT("S47WalkFrom"),TEXT("S47Auto"),TEXT("S47Ch3Break")})
  {FString V;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),V)){Finish(false,TEXT("another independent input/scenario driver is enabled"));return;}}
  FString Raw,ResultRaw;const FString ResultPath=FPaths::GetPath(PriorPath)/TEXT("result.json");const bool Loaded=FFileHelper::LoadFileToString(Raw,*PriorPath)&&FFileHelper::LoadFileToString(ResultRaw,*ResultPath);const auto Prior=Loaded?S47J::Read(Raw):nullptr,Result=Loaded?S47J::Read(ResultRaw):nullptr;
  const auto File=SC08File(PriorPath),Pinned=S47J::Obj(Result,TEXT("checkpoint_file")),C=S47J::Obj(S47J::Obj(Prior,TEXT("case")),TEXT("ch2"));
  if(!Check(Prior&&Result&&File&&Pinned&&S47J::Str(Result,TEXT("status"))==TEXT("PASS")&&S47J::Str(Result,TEXT("scenario"))==TEXT("SC07"),TEXT("actual named SC07 PASS result and complete checkpoint are readable"))||!Check(S47J::Str(Pinned,TEXT("path"))==PriorPath&&S47J::Num(Pinned,TEXT("bytes"))==S47J::Num(File,TEXT("bytes"))&&S47J::Str(Pinned,TEXT("sha1"))==S47J::Str(File,TEXT("sha1")),TEXT("actual SC07 PASS pins these exact prerequisite checkpoint bytes")))return;
  ChainId=S47J::Str(Result,TEXT("chain_id"));if(!Check(!ChainId.IsEmpty(),TEXT("continue the exact non-empty predecessor chain_id; never invent a new chain")))return;
  if(!Check(S47J::Int(C,TEXT("sc07Version"))==1&&S47J::Bool(C,TEXT("binderCompared"))&&S47J::Bool(C,TEXT("p04"))&&S47J::Bool(C,TEXT("p05"))&&S47J::Bool(C,TEXT("called"))&&S47J::Bool(S47J::Obj(Prior,TEXT("office")),TEXT("departed"))&&S47J::Int(S47J::Obj(Prior,TEXT("spine")),TEXT("sc08"))==1,TEXT("legitimate actual corrected SC07 and physical Office departure, no substitute progress")))return;
  for(const auto& V:S47J::Arr(C,TEXT("sources")))if(V&&V->Type==EJson::Object){const auto D=V->AsObject();Papers.Add(S47J::Str(D,TEXT("id")),S47J::Write(D));}
  for(const TCHAR* Id:{TEXT("obs_runbinder"),TEXT("EV_DALE_INDEX"),TEXT("EV_DALE_NOTE"),TEXT("p04"),TEXT("p05"),TEXT("access")})if(!Check(Papers.Contains(Id),FString(TEXT("actual predecessor owns complete source: "))+Id))return;
  PriorReceipt=S47J::New();S47J::Set(PriorReceipt,TEXT("checkpoint"),File);S47J::Set(PriorReceipt,TEXT("result"),SC08File(ResultPath));
  US47Flow::FStart Start;Start.State=Prior;Start.bDev=true;M->Flow->StartGame(Start);Next(1,.6);return;
 }
 if(FPlatformTime::Seconds()-Began>1000){Finish(false,TEXT("1000-second SC08 wall timeout"));return;}
 if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy)return;if(Wait>0){Wait-=Dt;return;}
 if(!Pending.IsEmpty()&&!Restored())return;if(M->Mode!=US47Main::EMode::Play)return;auto* A=Act();if(!A){Finish(false,TEXT("new SC08 owner missing after legitimate prerequisite"));return;}if(A->bUnavailable){Finish(false,TEXT("required original array boards/body unavailable"));return;}
 if(A->HuntLive())SearchRead|=A->Behavior==FS47SC08::EBody::Search;
 const bool Long=Mode==TEXT("sc08long"),Retry=Mode==TEXT("sc08retry");
 switch(Stage)
 {
 case 1:
  if(!Check(A->Phase()==FS47SC08::EStep::Earl&&M->Spine->Gate(FS47Spine::SC08)==1&&M->Story->ChapterKey==TEXT("ch2")&&!M->Story->Begun.Contains(TEXT("ch3"))&&M->Story->Clock()>=3*3600+55*60&&M->Story->Clock()<4*3600,TEXT("actual SC07 ends at03:55; pending SC08 creates no premature clock/fact"))||!Documents())return;
  if(!M->Office->Ready()||!A->bBuilt)return;Plan({.10,8.1},2);return;
 case 2:if(Walk()){if(Face(TEXT("office:arrayService")))Next(3);}return;
 case 3:if(!Use(TEXT("office:arrayService")))return;Next(4);return;
 case 4:
 {
  const auto* D=M->Story->Docs().FindByPredicate([](const FS47Doc& P){return P.Id==TEXT("EV_DALE_ARRAY_SERVICE");});
  if(!Check(M->ModalOpen()&&D&&D->Page.Contains(TEXT("ROW 1"))&&D->Page.Contains(TEXT("REF")),TEXT("physical service sheet exposes actual A/B/REF coordinates; INDEX is a separate source")))return;Shot(TEXT("row-one-service-original"));Next(5,.4);return;
 }
 case 5:M->UI->Close(true);if(!Store(TEXT("before-earl")))return;Next(6);return;
 case 6:Plan({-5.4,5.7},7);return;
 case 7:if(Walk()){if(Face(TEXT("exitDoor")))Next(8);}return;
 case 8:if(!M->Web->IsDoorOpen(TEXT("exitDoor"))&&!Use(TEXT("exitDoor")))return;Next(9,.8);return;
 case 9:Approach(TEXT("truck"),10);return;
 case 10:if(Walk()){if(Face(TEXT("truck")))Next(11);}return;
 case 11:
  OdoStart=M->World->Odometer();M->World->Autopilot=18.;if(!Use(TEXT("truck"))||!Check(M->World->bDriving&&M->Story->Clock()>=4*3600,TEXT("ordinary HUD truck interaction begins actual SC08 trip at04:00")))return;Next(12,0);return;
 case 12:
  if(M->World->bDriving){if(FPlatformTime::Seconds()-Mark>240)Finish(false,TEXT("actual Earl-bound trip did not park"));return;}
  OdoOut=M->World->Odometer();if(!Check(M->World->Area==TEXT("diner"),TEXT("actual outward trip arrives in the diner/town world area"))||!Check(M->World->TruckAt==TEXT("diner")&&!M->World->bCommuteVehicle&&(OdoOut-OdoStart)*1609.344>600,TEXT("actual road trip/parking/ordinary get-out; service odometer advances")))return;Next(13,.6);return;
 case 13:Approach(TEXT("town:amDoor"),14);return;
 case 14:if(Walk()){if(Face(TEXT("town:amDoor")))Next(15);}return;
 case 15:if(!Use(TEXT("town:amDoor"))||!Check(A->bDialog&&A->Plays==0,TEXT("actual public studio door opens new isolated-reel choices after04:00")))return;A->Choice(TEXT("label"));A->Choice(TEXT("live"));if(!Check(A->bLabelRead&&A->Plays==0&&!A->bPlaying,TEXT("actual OFF AIR label; live monitor cannot count as isolated reel")))return;Shot(TEXT("earl-off-air-label"));A->Choice(TEXT("isolated"));Next(16,0);return;
 case 16:
  if(A->bPlaying){if(FPlatformTime::Seconds()-Mark>95)Finish(false,TEXT("first original reel did not complete"));return;}
  if(!Check(A->Plays==1&&M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("witness-reel");}),TEXT("first complete original transport files actual witness-reel")))return;
  A->Choice(TEXT("repeat"));Next(17,0);return;
 case 17:
  if(A->bPlaying){if(FPlatformTime::Seconds()-Mark>95)Finish(false,TEXT("repeat original reel did not complete"));return;}
  if(!Check(A->Plays==2&&A->Phase()==FS47SC08::EStep::Array&&A->SourcePapers.ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc08_reel_cues")&&D.Page.Contains(TEXT("boy"))&&D.Page.Contains(TEXT("hole"));}),TEXT("new question produced the exact same original cue sequence on second COMPLETE play")))return;Snapshot(TEXT("two-identical-original-plays"));Shot(TEXT("earl-original-repeat"));Next(18,.4);return;
 case 18:M->UI->Close(true);if(!Store(TEXT("after-earl-two-original-plays")))return;Next(19);return;
 case 19:Approach(TEXT("dinerTruck"),20);return;
 case 20:if(Walk()){if(Face(TEXT("dinerTruck")))Next(21);}return;
 case 21:M->World->Autopilot=18.;if(!Use(TEXT("dinerTruck"))||!Check(M->World->bDriving,TEXT("ordinary diner truck interaction begins actual SARO return")))return;Next(22,0);return;
 case 22:
  if(M->World->bDriving){if(FPlatformTime::Seconds()-Mark>240)Finish(false,TEXT("actual return trip did not park"));return;}
  if(!Check(M->World->Area==TEXT("saro")&&M->World->TruckAt==TEXT("saro")&&(M->World->Odometer()-OdoOut)*1609.344>600,TEXT("actual return drive/parking/get-out preserves original sources")))return;Next(23,.6);return;
 case 23:Plan({-1,-17.5},24);return;
 case 24:if(Walk()){if(Face(TEXT("fieldGate")))Next(25);}return;
 case 25:if(!M->Web->IsDoorOpen(TEXT("fieldGate"))&&!Use(TEXT("fieldGate")))return;Next(26,.8);return;
 case 26:Path({{-1,-22},{-1,-42},{-1,-52},{-1,-63},{-1,-74}},27);return;
 case 27:if(Walk()){if(Face(TEXT("sc08:ref")))Next(28);}return;
 case 28:
  if(!Use(TEXT("sc08:ref")))return;A->Choice(TEXT("isolate"));A->Choice(TEXT("reference"));A->Choice(Long?TEXT("first-b"):TEXT("first-a"));
  if(!Check(A->bIsolated&&A->bRefRecorded&&A->FixedReference()&&A->NoCommand()&&A->Phase()==FS47SC08::EStep::Compare,TEXT("ordinary local isolation + actual fixed REF; no new console command")))return;M->UI->Close(true);Plan(FS47SC08::Point(A->First),29);return;
 case 29:if(Walk()){if(Face(Long?TEXT("sc08:b"):TEXT("sc08:a")))Next(30,.3);}return;
 case 30:
  if(!A->Settled()||A->Still<.8){if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("actual first encoders did not settle"));return;}
  if(!Use(Long?TEXT("sc08:b"):TEXT("sc08:a"))||!Check(A->Samples.Num()==1&&A->NoCommand()&&A->FixedReference(),TEXT("first actual selected station encoder record")))return;FirstNative=A->Readback();Snapshot(TEXT("first-station-encoders"));Shot(TEXT("array-first-station"));Next(31,.3);return;
 case 31:if(!Store(TEXT("one-actual-encoder-record")))return;Next(32);return;
 case 32:Plan(FS47SC08::Point(Long?TEXT("A"):TEXT("B")),33);return;
 case 33:if(Walk()){if(Face(Long?TEXT("sc08:a"):TEXT("sc08:b")))Next(34,.3);}return;
 case 34:
  if(!A->Settled()||A->Still<.8){if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("actual second encoders did not settle"));return;}
  if(!Use(Long?TEXT("sc08:a"):TEXT("sc08:b"))||!Check(A->Samples.Num()==2,TEXT("operator moves to other station; REF unchanged")))return;SecondNative=A->Readback();
  if(!Check(S47J::Arr(FirstNative,TEXT("actual_native_dishes")).Num()==3&&S47J::Arr(SecondNative,TEXT("actual_native_dishes")).Num()==3,TEXT("actual three native instanced dish readbacks exist")))return;
  for(int32 I=0;I<3;++I)
  {
   const auto One=S47J::Arr(FirstNative,TEXT("actual_native_dishes"))[I]->AsObject(),Two=S47J::Arr(SecondNative,TEXT("actual_native_dishes"))[I]->AsObject();const auto P1=S47J::Arr(One,TEXT("native_parts")),P2=S47J::Arr(Two,TEXT("native_parts"));
   if(!Check(P1.Num()==2&&P2.Num()==2&&S47J::Str(P1[0]->AsObject(),TEXT("world_transform"))!=S47J::Str(P2[0]->AsObject(),TEXT("world_transform")),TEXT("actual native azimuth-bearing instance changed with operator, independently of UI numbers")))return;
  }
  Snapshot(TEXT("second-station-native-encoders"));Shot(TEXT("array-second-station"));Plan(FS47SC08::Point(A->First),35);return;
 case 35:if(Walk()){if(Face(Long?TEXT("sc08:b"):TEXT("sc08:a")))Next(36,.3);}return;
 case 36:
  if(!A->Settled()||A->Still<.8){if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("return encoders did not settle"));return;}
  if(!Use(Long?TEXT("sc08:b"):TEXT("sc08:a"))||!Check(A->bCompared&&A->Samples.Num()==3&&A->SourcesReady()&&M->ModalOpen()&&A->NoCommand()&&A->FixedReference(),TEXT("actual selected A/B/return is repeatable; complete comparison paper, fixed reference, independent unchanged command history")))return;Shot(TEXT("array-original-comparison"));Next(37,.4);return;
 case 37:M->UI->Close(true);Next(38,.1);return;
 case 38:if(!Store(TEXT("actual-comparison-hunt-pending")))return;Next(39,.4);return;
 case 39:
  if(!Check(A->HuntLive()&&!A->bEscaped&&A->SourcesReady(),TEXT("full Flow cannot award escape for saved pending hunt")))return;CaptureBefore=A->Captures;ClockBeforeCapture=M->Story->Clock();Next(40,0);return;
 case 40:
  if(A->bShown&&A->Pale.Torso.IsValid())FacePoint(S47Three::ToThree(A->Pale.Torso->Bounds.Origin));
  if(!BodyWitness&&RenderBody()){Snapshot(TEXT("actual-field-body"));Shot(TEXT("pale-body-between-footings"));}
  if(!BodyWitness){if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("no genuine legal-camera native-bounds/clear-opaque-sight/recent-render witness"));return;}
  if(Retry){Next(41,0);return;}
  if(!Check(SearchRead,TEXT("actual field search was seen before choosing crossing")))return;
  if(Long)Path({{-1,-74},{-1,-63},{-1,-52},{-10.45,-52},{-10.45,-56.3},{-20,-56.3},{-20,-47.7},{-10.45,-47.7},{-10.45,-52},{-1,-52},{-1,-42},{4,-42},{4,-47.5},{14.8,-47.5},{14.8,-36.8},{4,-36.8},{4,-42},{-1,-42},{-1,-22},{-1,-17.5}},47);
  else Path({{-1,-74},{-1,-63},{-1,-52},{-1,-42},{-1,-22},{-1,-17.5}},47);return;
 case 41:
  if(A->Captures!=CaptureBefore){Finish(false,TEXT("capture before explicit run-away negative"));return;}
  if(A->Behavior==FS47SC08::EBody::Grab&&A->StateT<.35){Snapshot(TEXT("actual-grab-before-running"));Path({{-4,-74}},42);return;}
  if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("no actual Grab state at reachable stationary player"));return;
 case 42:
  if(A->Captures!=CaptureBefore){Finish(false,TEXT("Grab captured player after actual run-away"));return;}
  if(Walk())
  {
   if(!Check(A->Behavior!=FS47SC08::EBody::Grab&&FVector2D::Distance(A->Pos,FVector2D(M->Player->Pos.X,M->Player->Pos.Z))>.9,TEXT("running out of actual reach cancels unfinished Grab")))return;GrabNegative=true;Next(43,0);
  }return;
 case 43:
  if(A->Captures==CaptureBefore){if(FPlatformTime::Seconds()-Mark>35)Finish(false,TEXT("stationary reachable player was never actually caught"));return;}
  if(!Check(A->Captures==CaptureBefore+1&&!A->bEscaped&&M->Story->Clock()>=ClockBeforeCapture&&A->Samples.Num()==3&&A->SourcesReady(),TEXT("actual capture performs local retry only; no clock regression/source/control loss"))||!Documents())return;Snapshot(TEXT("actual-local-capture-retry"));Next(44,1.5);return;
 case 44:if(!Store(TEXT("after-actual-capture-retry")))return;Next(45,.5);return;
 case 45:if(!Check(A->HuntLive()&&!A->bEscaped&&A->Captures==CaptureBefore+1,TEXT("retry disk retains actual capture and does not invent escape")))return;Path({{-1,-74},{-1,-63},{-1,-52},{-1,-42},{-1,-22},{-1,-17.5}},47);return;
 case 47:
  if(A->Captures!=CaptureBefore+(Retry?1:0)){Finish(false,TEXT("unexpected actual capture on chosen physical escape route"));return;}
  if(Walk())Next(48,0);return;
 case 48:
  if(!Check(A->bEscaped&&A->Phase()==FS47SC08::EStep::Hold&&A->EscapedAt>=ClockBeforeCapture&&!M->World->bDriving&&M->Player->Pos.Z>=-18.7,TEXT("only actual physical crossing credits field escape with originals")))return;
  if(Long&&!Check(CoverWitness,TEXT("long route actually broke body sight at an opaque native footing")))return;Snapshot(TEXT("actual-field-escape-hold"));Shot(TEXT("field-gate-actual-escape"));Next(49,.2);return;
 case 49:if(!Store(TEXT("actual-escaped-hold")))return;Next(50,.3);return;
 case 50:
  if(M->Story->IsCinematic()||M->Spine->Gate(FS47Spine::SC08)!=2||!M->Story->Begun.Contains(TEXT("ch3"))){if(FPlatformTime::Seconds()-Mark>100)Finish(false,TEXT("actual return-to026/original chapter card did not complete"));return;}
  if(!Check(A->Phase()==FS47SC08::EStep::Done&&A->bEscaped&&M->Story->ChapterKey==TEXT("ch3")&&M->Story->Clock()>=4*3600+25*60&&M->Story->Clock()<4*3600+27*60&&!A->bShown,TEXT("real escape/home026 alone releases original Chapter2 card into SC09 at04:25; no SC09 action")))return;
  for(int32 I:A->Dishes)if(!Check(FMath::Abs(M->Web->DishAz(I)-26)<.05&&FMath::Abs(M->Web->DishEl(I)-32)<.05,TEXT("actual named native encoders reached026/32 after holding last field point")))return;
  if(!Store(TEXT("sc08-complete"),true))return;Next(51);return;
 case 51:
  if(!Documents())return;M->Flow->Saves=FS47SaveStore(Root/TEXT("normal-save"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);Next(52);return;
 case 52:
 {
  if(!M->Flow->SaveBlock().IsEmpty()){if(FPlatformTime::Seconds()-Mark>25)Finish(false,TEXT("ordinary manual save remains blocked"));return;}
  FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Manual=Meta?Reader.Load(Meta->Id):nullptr;
  if(!Check(Manual&&FFileHelper::SaveStringToFile(S47J::Write(Manual,true),*(Root/TEXT("manual-before-title.disk.json"))),TEXT("ordinary isolated case1 manual save + independent disk reader: ")+Error))return;
  M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Choice=TitleReader.ContinueSave(),ActualChoice=M->Flow->Saves.ContinueSave();Expected=Choice?TitleReader.Load(Choice->Id):nullptr;
  if(!Check(M->Mode==US47Main::EMode::Title&&Choice&&ActualChoice&&Choice->Id==ActualChoice->Id&&Expected&&FFileHelper::SaveStringToFile(S47J::Write(Expected,true),*(Root/TEXT("normal-continue.disk.json"))),TEXT("fresh reader pins actual post-title Continue disk selection")))return;
  if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens actual case1 disk")))return;Pending=TEXT("normal-title-continue");++Stores;NormalContinue=true;Next(53,.6);return;
 }
 case 53:if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&A->Phase()==FS47SC08::EStep::Done&&A->bEscaped&&A->SourcesReady()&&M->Story->ChapterKey==TEXT("ch3"),TEXT("normal Continue preserves true completed SC08 and genuine SC09 start")))return;Snapshot(TEXT("normal-title-continue"));Shot(TEXT("after-normal-continue"));Next(54,.5);return;
 case 54:
  for(const FString& File:Pictures){S47J::FObj Png;if(!SC08Png(File,Png)&&FPlatformTime::Seconds()-Mark<8)return;}
  for(const FString& File:Pictures){S47J::FObj Png;if(!Check(SC08Png(File,Png),TEXT("actual PNG CRC/IEND/EOF/SHA: ")+File))return;Photos.Add(MakeShared<FJsonValueObject>(Png));}
  if(!Check(Stores==(Retry?8:7)&&Restores.Num()==Stores&&Uses>=11&&WalkSamples>300&&Walked>150&&Pictures.Num()==9&&BodyWitness&&SearchRead&&NormalContinue&&(!Long||CoverWitness)&&(!Retry||GrabNegative),TEXT("all actual prerequisite/physical trip/reel/control/hunt/disk/title/PNG steps executed")))return;
  Finish(true,TEXT("actual Earl reel, fixed-reference comparison, reactive array route and source-preserving SC09 continuation"));return;
 default:Finish(false,TEXT("unknown SC08 contract stage"));return;
 }
}
void FS47SC08Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC08Contract> Run;
 if(Owner.Get()!=Main){Owner=Main;Run=MakeShareable(new FS47SC08Contract(Main));}const auto KeepAlive=Run;if(KeepAlive)KeepAlive->Tick(Dt);
}
