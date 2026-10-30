#include "S47SC05Contract.h"
#include "S47Audio.h"
#include "S47Chapter1.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47SC05.h"
#include "S47Seen.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47ThreatNavigation.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47ScreenView.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 using FJ=S47J::FObj;
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 FString Hash(const FString& Path)
 {
  TArray<uint8> Bytes;if(Path.IsEmpty()||!FFileHelper::LoadFileToArray(Bytes,*Path))return {};
  FSHAHash H;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),H.Hash);return H.ToString();
 }
 FJ Read(const FString& Path){FString Text;return FFileHelper::LoadFileToString(Text,*Path)?S47J::Read(Text):FJ();}
 FString Full(FString Path){Path=FPaths::ConvertRelativePathToFull(Path);FPaths::CollapseRelativeDirectories(Path);FPaths::NormalizeFilename(Path);return Path;}
 void Texts(const TSharedRef<SWidget>& Widget,TArray<FString>& Out)
 {
  if(!Widget->GetVisibility().IsVisible())return;
  if(Widget->GetTypeAsString()==TEXT("STextBlock"))Out.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Kids=Widget->GetChildren();for(int32 I=0;Kids&&I<Kids->Num();++I)Texts(Kids->GetChildAt(I),Out);
 }
 // SC04-owned documents call the same numeric field paperRatio; routine calls it ratio.
 // Only this named representation difference is normalized. Raw source bytes are retained too.
 FJ CanonicalDoc(const FJ& Raw)
 {
  if(!Raw)return nullptr;auto D=S47J::New();
  for(const auto* Key:{TEXT("id"),TEXT("title"),TEXT("kind"),TEXT("stamp"),TEXT("page"),TEXT("transcript"),TEXT("image"),TEXT("paper")})S47J::Set(D,Key,S47J::Str(Raw,Key));
  S47J::Set(D,TEXT("ratio"),S47J::Num(Raw,S47J::Has(Raw,TEXT("paperRatio"))?TEXT("paperRatio"):TEXT("ratio")));
  D->SetArrayField(TEXT("sheets"),S47J::Arr(Raw,TEXT("sheets")));return D;
 }
 bool Png(const FString& Path,FJ& Out)
 {
  TArray<uint8>B;if(!FFileHelper::LoadFileToArray(B,*Path))return false;
  const uint8 Sig[]={137,80,78,71,13,10,26,10};if(B.Num()<33||FMemory::Memcmp(B.GetData(),Sig,8))return false;
  const auto U32=[&](int64 I){return(uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Data=0;int32 Count=0;uint32 W=0,H=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 N=U32(At);if(int64(N)>B.Num()-At-12)return false;const auto* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,N+4)!=U32(At+8+N))return false;
   if(!Count){if(FMemory::Memcmp(Type,"IHDR",4)||N!=13)return false;W=U32(At+8);H=U32(At+12);}
   if(!FMemory::Memcmp(Type,"IDAT",4))Data+=N;++Count;At+=int64(N)+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=N==0;break;}
  }
  if(!End||At!=B.Num()||Data<=0||W<640||H<360)return false;
  Out=S47J::New();S47J::Set(Out,TEXT("path"),Path);S47J::Set(Out,TEXT("bytes"),B.Num());S47J::Set(Out,TEXT("sha1"),Hash(Path));
  S47J::Set(Out,TEXT("width"),double(W));S47J::Set(Out,TEXT("height"),double(H));S47J::Set(Out,TEXT("crc_iend_eof"),true);S47J::Set(Out,TEXT("human_visual_acceptance"),false);return true;
 }
}

class FS47SC05Contract : public TSharedFromThis<FS47SC05Contract>
{
 using FJ=S47J::FObj;
 TWeakObjectPtr<US47Main> Owner;
 US47Main* M()const{return Owner.Get();}
 FS47SC05* A()const{return M()&&M()->Spine?&M()->Spine->Call():nullptr;}
 FString Root,Input,InputResult,InputSHA,InputResultSHA,Target,PendingPhoto;
 FJ Original,PriorResult,Expected,PauseBefore,PhotoBefore;
 TMap<FString,FJ> Documents;
 TMap<FString,int32> Nodes;
 TArray<FVector2D> Route;
 TArray<TSharedPtr<FJsonValue>> Executed,Rays,States,Restores,Photos,Api,Blocked;
 TArray<TArray<double>> InitialPointing;
 FVector Aim=FVector::ZeroVector,LastWalk=FVector::ZeroVector;
 int32 Stage=0,Checks=0,Uses=0,Stores=0,TitleContinues=0,Cursor=0,WalkSamples=0;
 bool Started=false,Done=false,DidPause=false,NormalContinue=false;
 double Began=FPlatformTime::Seconds(),Mark=Began,Wait=0,Walked=0,CountdownWall=0,CountdownGt=-1,PauseWall=0,EventWall=0,AcquiredAt=-1,SolveAt=-1;
 bool Write(const FString& Name,const FJ& Value)const{return !Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(Value,true),*(Root/Name));}
 bool Copy(const FString& From,const FString& To)const{TArray<uint8>B;return FFileHelper::LoadFileToArray(B,*From)&&FFileHelper::SaveArrayToFile(B,*To)&&Hash(From)==Hash(To);}
 void Next(int32 Value,double Hold=0){Stage=Value;Wait=Hold;Mark=FPlatformTime::Seconds();}
 void Finish(bool Good,const FString& Why)
 {
  const auto Keep=AsShared();if(Done)return;Done=true;
  if(M()&&M()->Player){M()->Player->TestMove.Reset();M()->Player->bTestRun=false;}
  FString Reason=Why;
  if(!InputSHA.IsEmpty()&&(Hash(Input)!=InputSHA||Hash(InputResult)!=InputResultSHA)){Good=false;Reason+=TEXT(" / actual SC04 input bytes changed");}
  auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("scenario"),TEXT("SC05"));S47J::Set(O,TEXT("mode"),TEXT("sc05"));
  S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("reason"),Reason);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stores"),Stores);S47J::Set(O,TEXT("actual_ray_uses"),Uses);
  S47J::Set(O,TEXT("walked_metres"),Walked);S47J::Set(O,TEXT("walk_samples"),WalkSamples);S47J::Set(O,TEXT("normal_title_continue"),NormalContinue);S47J::Set(O,TEXT("ordinary_title_continues"),TitleContinues);
  const FString Chain=S47J::Str(PriorResult,TEXT("chain_id"));if(!Chain.IsEmpty())S47J::Set(O,TEXT("chain_id"),Chain);
  S47J::Set(O,TEXT("source_prerequisite_fixtures"),S47J::Bool(PriorResult,TEXT("source_prerequisite_fixtures"),true));
  S47J::Set(O,TEXT("source_sc04_checkpoint"),Input);S47J::Set(O,TEXT("source_sc04_sha1"),InputSHA);S47J::Set(O,TEXT("source_sc04_result_sha1"),InputResultSHA);S47J::Set(O,TEXT("source_sc04_raw_result"),PriorResult);
  S47J::Set(O,TEXT("source_sc04_unchanged"),!InputSHA.IsEmpty()&&Hash(Input)==InputSHA&&Hash(InputResult)==InputResultSHA);
  S47J::Set(O,TEXT("actual_room_game_seconds"),A()?A()->ActualInterval():-1);S47J::Set(O,TEXT("dead_line_to_event_wall_seconds"),EventWall>0?EventWall-CountdownWall:0);S47J::Set(O,TEXT("pause_wall_seconds"),PauseWall);
  S47J::Set(O,TEXT("final_facts"),A()?A()->Save():FJ());S47J::Set(O,TEXT("sc05_complete_checkpoint"),Root.IsEmpty()?FString():Root/TEXT("sc05-complete.disk.json"));
  const FString Complete=Root.IsEmpty()?FString():Root/TEXT("sc05-complete.disk.json");
  if(!Complete.IsEmpty()&&IFileManager::Get().FileExists(*Complete))
  {auto File=S47J::New();S47J::Set(File,TEXT("path"),Complete);S47J::Set(File,TEXT("bytes"),double(IFileManager::Get().FileSize(*Complete)));S47J::Set(File,TEXT("sha1"),Hash(Complete));S47J::Set(O,TEXT("checkpoint_file"),File);}
  S47J::Set(O,TEXT("method"),TEXT("Actual SC04 PASS/checkpoint input, ordinary case1 disk/title/Continue. Native Walker TestMove/Resolve and actual camera Pick/Update/HUD OnUse. Console dial edits/action and offered CRT EXIT are explicitly API. Real 47 game seconds, ordinary pause5wallseconds; no clock/Event/Place or prior-fact fixture."));
  S47J::Set(O,TEXT("limits"),Chain.IsEmpty()?TEXT("The actual SC04 result's prior fixture scope is inherited unchanged and never accepted as a complete night. Audio route/context and PNGs do not grant human visual/input/listening/performance/installed acceptance."):TEXT("The actual same-night SC03/SC04 chain and all parent bytes are inherited unchanged. Audio route/context and PNGs do not grant human visual/input/listening/performance/installed acceptance. SC06-SC13 are not executed by this driver."));
  S47J::Set(O,TEXT("human_visual_acceptance"),false);S47J::Set(O,TEXT("human_input"),false);S47J::Set(O,TEXT("installed"),false);
  double OuterSeconds=0;FParse::Value(FCommandLine::Get(),TEXT("S47Seconds="),OuterSeconds);
  S47J::Set(O,TEXT("smoke_focus_and_outer_timer"),FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest")));S47J::Set(O,TEXT("outer_timer_seconds"),OuterSeconds);
  S47J::Set(O,TEXT("outer_timer_is_contract_pass"),false);
  O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("rays"),Rays);O->SetArrayField(TEXT("states"),States);O->SetArrayField(TEXT("restores"),Restores);
  O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("api_actions"),Api);O->SetArrayField(TEXT("blocked_save_attempts"),Blocked);
  const bool Saved=Write(TEXT("result.json"),O);
  if(Good&&Saved){UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc05 PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm evidence=%s"),Checks,Stores,Uses,Walked,*Root);}
  else{UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc05 FAIL stage %d: %s (result written %d)"),Stage,*Reason,Saved);}
  FPlatformMisc::RequestExitWithStatus(false,Good&&Saved?0:1);
 }
 bool Check(bool Good,const FString& Why)
 {
  ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Checks);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("detail"),Why);Executed.Add(MakeShared<FJsonValueObject>(O));
  if(!Good){Finish(false,Why);return false;}return true;
 }
 TArray<TArray<double>> Pointing()const
 {
  TArray<TArray<double>> Out;for(int32 I=0;I<M()->Web->DishCount();++I)Out.Add({M()->Web->DishAz(I),M()->Web->DishEl(I)});return Out;
 }
 FJ Measurement()const
 {
  auto O=S47J::New();S47J::Set(O,TEXT("game_time"),M()->Story->Gt());S47J::Set(O,TEXT("clock"),M()->Story->Clock());S47J::Set(O,TEXT("phase"),int32(M()->Story->Phase()));
  S47J::Set(O,TEXT("call_raw"),A()->Save());S47J::Set(O,TEXT("rx_stage"),M()->Story->Rx.Stage);S47J::Set(O,TEXT("mug_visible"),M()->Web->IsMugVisible());
  TArray<TSharedPtr<FJsonValue>> Ps;for(const auto& P:Pointing()){TArray<TSharedPtr<FJsonValue>> R;for(double V:P)R.Add(MakeShared<FJsonValueNumber>(V));Ps.Add(MakeShared<FJsonValueArray>(R));}O->SetArrayField(TEXT("actual_pointing"),Ps);return O;
 }
 bool Sources(const FString& Context,bool Remember=false)
 {
  TMap<FString,FJ> Current;for(const auto& V:S47J::Arr(M()->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object)
  {const auto D=CanonicalDoc(V->AsObject());const FString Id=S47J::Str(D,TEXT("id"));if(!Check(!Id.IsEmpty()&&!Current.Contains(Id),Context+TEXT(" unique actual document ID: ")+Id))return false;Current.Add(Id,D);}
  for(const auto& P:Documents)if(!Check(Current.Contains(P.Key)&&Same(P.Value,Current[P.Key]),Context+TEXT(" full acquired document/sheet bytes unchanged: ")+P.Key))return false;
  if(Remember)for(const auto& P:Current)Documents.Add(P.Key,S47J::Clone(P.Value));return true;
 }
 bool Others(const FString& Context,bool IncludeS03=false)
 {
  const auto Now=Pointing();if(!Check(Now.Num()==InitialPointing.Num()&&Now.Num()==27,Context+TEXT(" actual27 dish inventory")))return false;
  for(int32 I=0;I<Now.Num();++I)if(I!=2||IncludeS03)
   if(!Check(FMath::Abs(Now[I][0]-InitialPointing[I][0])<1e-6&&FMath::Abs(Now[I][1]-InitialPointing[I][1])<1e-6,Context+FString::Printf(TEXT(" actual held encoder S-%02d unchanged"),I+1)))return false;
  return true;
 }
 bool Snapshot(const FString& Name)
 {
  auto O=Measurement();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("full_game_state"),M()->Flow->GameState());S47J::Set(O,TEXT("actual_camera_eye_ue_cm"),M()->Player->EyeUE().ToString());
  S47J::Set(O,TEXT("actual_camera_forward_ue"),M()->Player->ForwardUE().ToString());S47J::Set(O,TEXT("player_three_m"),M()->Player->Pos.ToString());
  S47J::Set(O,TEXT("audio_context_available"),M()->Audio&&M()->Audio->Ctx.IsValid());
  if(M()->Audio&&M()->Audio->Ctx){S47J::Set(O,TEXT("audio_context_time"),M()->Audio->Ctx->CurrentTime());S47J::Set(O,TEXT("audio_context_state"),M()->Audio->Ctx->State());}
  S47J::Set(O,TEXT("save_block"),M()->Flow->SaveBlock());States.Add(MakeShared<FJsonValueObject>(O));return Check(Write(Name+TEXT(".raw.json"),O),TEXT("retain unmodified raw phase evidence: ")+Name);
 }
 bool Shot(const FString& Name)
 {
  const FString File=Root/(Name+TEXT(".png"));
  if(PendingPhoto.IsEmpty())
  {
   if(!Check(!IFileManager::Get().FileExists(*File)&&!FScreenshotRequest::IsScreenshotRequested(),TEXT("fresh own viewport request: ")+Name))return false;
   PendingPhoto=Name;PhotoBefore=Measurement();S47J::Set(PhotoBefore,TEXT("request_frame"),double(GFrameCounter));FScreenshotRequest::RequestScreenshot(File,true,false);return false;
  }
  if(PendingPhoto!=Name){Finish(false,TEXT("overlapping screenshot request"));return false;}
  FJ Proof;if(!Png(File,Proof)){if(FPlatformTime::Seconds()-Mark>12)Finish(false,TEXT("viewport PNG incomplete: ")+Name);return false;}
  S47J::Set(Proof,TEXT("request_measurement"),PhotoBefore);S47J::Set(Proof,TEXT("completed_frame"),double(GFrameCounter));Photos.Add(MakeShared<FJsonValueObject>(Proof));PendingPhoto.Reset();PhotoBefore.Reset();return true;
 }
 bool Restored(const FString& Name)
 {
  const auto Now=M()->Flow->GameState(),SavedRoutine=S47J::Obj(Expected,TEXT("routine")),SavedCase=S47J::Obj(Expected,TEXT("case")),Pose=S47J::Obj(Expected,TEXT("pose"));
  const double Clock=SavedRoutine?S47J::Num(SavedRoutine,TEXT("clock")):S47J::Num(SavedCase,TEXT("clock"));
  if(!Check(M()->Mode==US47Main::EMode::Play&&M()->Flow->CaseId==1&&!M()->Flow->bDevRun&&M()->Evening->NightSequence()&&M()->Spine->Gate(FS47Spine::SC04)==2&&A()->Enabled(),TEXT("ordinary real case1/fullFlow owns actual SC04→SC05: ")+Name))return false;
  if(!Check(M()->Flow->CheckpointName()==S47J::Str(Expected,TEXT("checkpoint"))&&M()->World->Area==S47J::Str(Expected,TEXT("area"))
   &&Pose&&M()->Player->Walkable(S47J::Num(Pose,TEXT("x")),S47J::Num(Pose,TEXT("z")))&&FMath::Abs(M()->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.0001&&FMath::Abs(M()->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.0001
   &&FMath::Abs(M()->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.00001&&FMath::Abs(M()->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.00001&&M()->Story->Clock()==Clock
   &&Same(S47J::Obj(Now,TEXT("doors")),S47J::Obj(Expected,TEXT("doors"))),TEXT("synchronous normal Continue retains exact clock/pose/doors without test placement: ")+Name))return false;
  const auto ExpectedSpine=S47J::Obj(Expected,TEXT("spine")),ActualSpine=S47J::Obj(Now,TEXT("spine"));
  if(!Check(Same(S47J::Obj(ExpectedSpine,TEXT("sc04d")),S47J::Obj(ActualSpine,TEXT("sc04d"))),TEXT("full SC04 custody/escape/source bytes retained: ")+Name))return false;
  if(const auto SavedCall=S47J::Obj(ExpectedSpine,TEXT("sc05d"));SavedCall&&!Check(Same(SavedCall,A()->Save()),TEXT("full SC05 room/source/pointing state retained: ")+Name))return false;
  if(SavedRoutine)
  {
   const auto R=M()->Story->SaveRoutine();
   for(const auto* Key:{TEXT("phase"),TEXT("rxStage"),TEXT("f"),TEXT("g"),TEXT("bw"),TEXT("az"),TEXT("status"),TEXT("solved"),TEXT("solveAt"),TEXT("printoutAt")})
   {
    const auto E=SavedRoutine->TryGetField(Key),V=R->TryGetField(Key);
    if(!Check((!E&&!V)||(E&&V&&FJsonValue::CompareEqual(*E,*V)),Name+TEXT(" actual stable receiver field: ")+Key))return false;
   }
  }
  if(!Sources(Name)||!Others(Name))return false;
  auto Row=S47J::New();S47J::Set(Row,TEXT("name"),Name);S47J::Set(Row,TEXT("pass"),true);S47J::Set(Row,TEXT("full_raw_state"),Now);Restores.Add(MakeShared<FJsonValueObject>(Row));LastWalk=M()->Player->Pos;return true;
 }
 bool Cycle(const FString& Name,bool Complete=false)
 {
  M()->Player->TestMove.Reset();if(M()->ModalOpen()||M()->Story->IsCinematic()||M()->World->bBusy||M()->Flow->bStarting||!M()->Flow->SaveBlock().IsEmpty())
  {if(FPlatformTime::Seconds()-Mark>25)Finish(false,TEXT("ordinary save remains blocked at ")+Name+TEXT(": ")+M()->Flow->SaveBlock());return false;}
  if(!Check(M()->Flow->CaseId==1&&!M()->Flow->bDevRun,TEXT("manual save uses actual isolated case1: ")+Name))return false;
  Expected=S47J::Clone(M()->Flow->GameState());
  if(!Check(Snapshot(Name+TEXT("-stable-before-manual")),TEXT("named stable raw snapshot before disk/title: ")+Name))return false;
  FString Error;const auto Meta=M()->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):FJ();
  if(!Check(Meta&&Same(Disk,Expected),TEXT("ordinary manual and independent complete disk reader: ")+Name+TEXT(" / ")+Error)||!Check(Write(Name+TEXT(".disk.json"),Disk),TEXT("retain exact complete disk state: ")+Name))return false;
  if(Complete&&!Check(Name==TEXT("sc05-complete")&&A()->Done()&&S47J::Str(Disk,TEXT("checkpoint"))==TEXT("chapter1"),TEXT("stable actual SC05 checkpoint for next scenario before title/Continue")))return false;
  ++Stores;M()->Flow->ToTitle();FS47SaveStore TitleReader(M()->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),FlowSelected=M()->Flow->Saves.ContinueSave();
  const auto Chosen=Selected?TitleReader.Load(Selected->Id):FJ();
  if(!Check(M()->Mode==US47Main::EMode::Title&&Selected&&FlowSelected&&Selected->Id==FlowSelected->Id&&Same(Chosen,Disk),TEXT("ordinary title/fresh reader selects this actual full checkpoint: ")+Name))return false;
  Expected=Chosen;if(!Check(M()->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue API: ")+Name))return false;
  ++TitleContinues;NormalContinue=true;return Restored(Name);
 }
 bool BlockedSave(const FString& Name)
 {
  auto List=[](const TArray<FS47SaveMeta>& Items){auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> A;for(const auto& Meta:Items){auto R=S47J::New();Meta.Write(R);A.Add(MakeShared<FJsonValueObject>(R));}O->SetArrayField(TEXT("metas"),A);return O;};
  const auto Before=List(M()->Flow->Saves.List());const FString Block=M()->Flow->SaveBlock();FString Error;const bool Saved=M()->Flow->AutosaveNow(&Error);
  FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();const auto After=List(Reader.List());
  auto R=S47J::New();S47J::Set(R,TEXT("phase"),Name);S47J::Set(R,TEXT("actual_block"),Block);S47J::Set(R,TEXT("actual_error"),Error);S47J::Set(R,TEXT("autosave_return"),Saved);S47J::Set(R,TEXT("before"),Before);S47J::Set(R,TEXT("fresh_after"),After);Blocked.Add(MakeShared<FJsonValueObject>(R));
  return Check(M()->Flow->CaseId==1&&!M()->Flow->bDevRun&&!Block.IsEmpty()&&!Saved&&Error==Block&&Same(Before,After),TEXT("genuine call-phase save gate refuses write without bypass: ")+Name);
 }
 FS47Obj Object(const FString& Id)const{const auto* I=M()->Inter->Get(Id);if(I&&I->Object)return I->Object;const auto* V=M()->Web->Item(Id);return V?M()->Web->ObjAt(V->Node):FS47Obj();}
 double Range(const FString& Id)const{const auto* I=M()->Inter->Get(Id);if(I&&I->Range>0)return I->Range;const auto* V=M()->Web->Item(Id);return V&&V->Range>0?V->Range:2.4;}
 bool Segment(const FVector2D& From,const FVector2D& To)const
 {
  const auto Room=M()->Web->Rect(TEXT("room.bounds"));const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(From,To)/.04));
  for(int32 I=0;I<=N;++I){const auto P=FMath::Lerp(From,To,double(I)/N);if(!Room.Contains(P.X,P.Y)||!M()->Player->Walkable(P.X,P.Y))return false;}return true;
 }
 bool Go(const FString& Id)
 {
  const auto Obj=Object(Id);const auto Bounds=Obj.WorldBox();if(!Check(Obj&&Bounds.IsValid,TEXT("actual original item bounds for legal approach: ")+Id))return false;
  const FVector Center=Bounds.GetCenter();const FVector2D Start(M()->Player->Pos.X,M()->Player->Pos.Z);const auto Room=M()->Web->Rect(TEXT("room.bounds"));
  if(!Check(Room.bValid&&Room.Contains(Start.X,Start.Y)&&M()->Player->Walkable(Start.X,Start.Y),TEXT("saved/current pose is walkable; no Place recovery: ")+Id))return false;
  TArray<FVector2D> Seeds;for(double X=Room.MinX+.4;X<Room.MaxX;X+=1.5)for(double Z=Room.MinZ+.4;Z<Room.MaxZ;Z+=1.5)if(M()->Player->Walkable(X,Z))Seeds.Add({X,Z});
  TArray<FVector2D> Best;double Cost=1e100;
  const auto Candidate=[&](const FVector2D& P)
  {
   if(!Room.Contains(P.X,P.Y)||!M()->Player->Walkable(P.X,P.Y))return;
   const FVector Eye(P.X,M()->Player->FloorY+M()->Player->Eye,P.Y);const FVector D=Center-Eye;
   if(D.Size()>Range(Id)-.08||M()->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)return;
   auto Path=FS47ThreatNavigation::FindPath(Start,P,Seeds,M()->Web->Colliders(),M()->Player->Radius,[&](const FVector2D& A,const FVector2D& B){return Segment(A,B);});
   if(Path.IsEmpty())return;double Length=0;auto Prev=Start;for(const auto& Point:Path){Length+=FVector2D::Distance(Prev,Point);Prev=Point;}
   if(Length<Cost){Cost=Length;Best=MoveTemp(Path);}
  };
  Candidate(Start);for(double Radius:{.7,1.15,1.6,2.05})for(int32 I=0;I<16;++I){const double Angle=UE_DOUBLE_PI*I/8.;Candidate({Center.X+Radius*FMath::Cos(Angle),Center.Z+Radius*FMath::Sin(Angle)});}
  if(!Check(!Best.IsEmpty(),TEXT("bounded existing visibility-graph route has walkable segments and native item ray: ")+Id))return false;
  Route=MoveTemp(Best);Cursor=0;Target=Id;LastWalk=M()->Player->Pos;Mark=FPlatformTime::Seconds();return true;
 }
 bool Walk()
 {
  auto* P=M()->Player.Get();const double Distance=FVector2D(P->Pos.X-LastWalk.X,P->Pos.Z-LastWalk.Z).Size();
  if(!Check(Distance<.45&&P->Walkable(P->Pos.X,P->Pos.Z),TEXT("actual native Walker/Resolve stays continuous/walkable; no recovery placement")))return false;
  Walked+=Distance;++WalkSamples;LastWalk=P->Pos;
  if(!Route.IsValidIndex(Cursor)){P->TestMove.Reset();return true;}
  if(FPlatformTime::Seconds()-Mark>25){Finish(false,TEXT("actual Walker stalled approaching ")+Target);return false;}
  const FVector2D D=Route[Cursor]-FVector2D(P->Pos.X,P->Pos.Z);
  if(D.Size()<.055){P->TestMove.Reset();++Cursor;Mark=FPlatformTime::Seconds();return Cursor==Route.Num();}
  if(!Segment(FVector2D(P->Pos.X,P->Pos.Z),Route[Cursor])){Finish(false,TEXT("planned segment became blocked; no route/pose repair"));return false;}
  P->Yaw=FMath::Atan2(-D.X,-D.Y);P->Pitch=-.04;P->TestMove=FVector2D(0,-FMath::Clamp(D.Size()/.35,0.,1.));P->bTestRun=false;return false;
 }
 bool Face(const FString& Id)
 {
  M()->Player->TestMove.Reset();const auto Obj=Object(Id);const auto Bounds=Obj.WorldBox();
  if(!Check(Obj&&Bounds.IsValid&&(!Nodes.Contains(Id)||Nodes[Id]==Obj.I),TEXT("same actual original item node: ")+Id))return false;
  Nodes.Add(Id,Obj.I);const FVector Three=Bounds.GetCenter(),D=Three-S47Three::ToThree(M()->Player->EyeUE());Aim=S47Three::ToUE(Three);
  M()->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M()->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
 }
 bool Use(const FString& Id)
 {
  const FVector Eye=M()->Player->EyeUE(),Forward=M()->Player->ForwardUE();const FString Pick=M()->Inter->Pick(Eye,Forward),Focus=M()->Inter->Update(Eye,Forward);
  auto R=S47J::New();S47J::Set(R,TEXT("id"),Id);S47J::Set(R,TEXT("picked"),Pick);S47J::Set(R,TEXT("focus"),Focus);S47J::Set(R,TEXT("walker_focus"),M()->Player->FocusedId());S47J::Set(R,TEXT("eye_ue_cm"),Eye.ToString());
  S47J::Set(R,TEXT("forward_ue"),Forward.ToString());S47J::Set(R,TEXT("item_range_m"),Range(Id));S47J::Set(R,TEXT("target_distance_m"),FVector::Distance(Eye,Aim)/100.);S47J::Set(R,TEXT("method"),TEXT("actual camera Pick/Update -> bound ordinary HUD OnUse"));Rays.Add(MakeShared<FJsonValueObject>(R));
  if(!Check(!M()->ModalOpen()&&Pick==Id&&Focus==Id&&M()->Player->FocusedId()==Id&&M()->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual settled camera ray/focus/ordinary HUD OnUse: ")+Id))return false;
  M()->Player->OnUse(Pick);++Uses;return true;
 }
 bool Modal(const FString& Fragment)const
 {
  TArray<FString> Values;const auto W=M()->Hud->ModalWidget();if(W)Texts(W.ToSharedRef(),Values);return M()->ModalOpen()&&Values.ContainsByPredicate([&](const FString& Text){return Text.Contains(Fragment);});
 }
 bool Console(bool Correct)
 {
  const auto Widget=M()->Hud->ModalWidget();if(!Check(M()->Story->bConsoleOpen&&Widget&&Widget->GetTypeAsString()==TEXT("SS47ScreenView"),TEXT("actual physically opened receiver CRT view before explicit dialer API")))return false;
  const auto View=StaticCastSharedPtr<SS47ScreenView>(Widget);if(!Check(View->OnGlass(),TEXT("actual native CRT glass view, not a fallback overlay")))return false;
  auto& Rx=M()->Story->Rx;Rx.Frequency=Correct?1420.405:1420.110;Rx.Gain=Correct?82:55;Rx.Bandwidth=12;Rx.Azimuth=83;
  M()->Story->ConsoleAzimuth();auto R=S47J::New();S47J::Set(R,TEXT("method"),TEXT("ConsoleDialerAPI: same public Rx dials and production ConsoleAzimuth/ConsoleAction as the Slate controls; not real mouse input"));
  S47J::Set(R,TEXT("f"),Rx.Frequency);S47J::Set(R,TEXT("gain"),Rx.Gain);S47J::Set(R,TEXT("bandwidth"),Rx.Bandwidth);S47J::Set(R,TEXT("search_azimuth"),Rx.Azimuth);S47J::Set(R,TEXT("stage_before"),Rx.Stage);
  M()->Story->ConsoleAction();S47J::Set(R,TEXT("stage_after"),Rx.Stage);S47J::Set(R,TEXT("actual_status"),Rx.Status);Api.Add(MakeShared<FJsonValueObject>(R));
  if(!Check(Correct?(Rx.Stage==3&&M()->Story->Phase()==ES47Phase::Locked):(Rx.Stage==2&&M()->Story->Phase()==ES47Phase::Residual&&Rx.Status.StartsWith(TEXT("NO LOCK"))),Correct?TEXT("actual residual locks only at1420.405/82/BW12/search083"):TEXT("earlier notch1420.110/55/BW12 cannot lock the residual"))||!Others(TEXT("passive console"),true))return false;
  View->Leave();return true;
 }
 bool InputGate();
 bool Begin();
public:
 explicit FS47SC05Contract(US47Main* Main):Owner(Main){}
 void Tick(double Dt);
};

bool FS47SC05Contract::InputGate()
{
 FParse::Value(FCommandLine::Get(),TEXT("S47SC04Checkpoint="),Input);if(!Check(!Input.IsEmpty(),TEXT("required actual SC04 checkpoint argument, no earlier-fact fixture")))return false;
 Input=Full(Input);InputResult=FPaths::GetPath(Input)/TEXT("result.json");Original=Read(Input);PriorResult=Read(InputResult);InputSHA=Hash(Input);InputResultSHA=Hash(InputResult);
 const auto Spine=S47J::Obj(Original,TEXT("spine")),Facts=S47J::Obj(Spine,TEXT("sc04d")),Routine=S47J::Obj(Original,TEXT("routine"));
 const FString Mode=S47J::Str(PriorResult,TEXT("mode"));const auto ChecksDone=S47J::Arr(PriorResult,TEXT("executed_checks"));
 bool Passed=ChecksDone.Num()>=20;for(const auto& V:ChecksDone)Passed&=V&&V->Type==EJson::Object&&S47J::Bool(V->AsObject(),TEXT("pass"));
 if(!Check(FPaths::GetCleanFilename(Input)==TEXT("sc04-complete.disk.json"),TEXT("exact actual SC04 checkpoint filename"))||!Check(Original&&Routine&&Facts&&PriorResult&&!InputSHA.IsEmpty()&&!InputResultSHA.IsEmpty(),TEXT("actual checkpoint and sibling result readable with source SHA1")))return false;
 if(!Check(S47J::Str(PriorResult,TEXT("status"))==TEXT("PASS"),TEXT("SC04 sibling is actual PASS, never pending/failed output"))||!Check((Mode==TEXT("sc04")||Mode==TEXT("sc04hide")||Mode==TEXT("sc04long"))&&Passed&&S47J::Bool(PriorResult,TEXT("normal_title_continue"))
  &&S47J::Int(PriorResult,TEXT("stores"))>=4&&S47J::Int(PriorResult,TEXT("actual_ray_uses"))>=9&&Same(S47J::Obj(PriorResult,TEXT("final_facts")),Facts)
  &&Full(S47J::Str(PriorResult,TEXT("sc04_complete_checkpoint")))==Input,TEXT("actual SC04 PASS mode, executed checks, disk/title/Continue and exact final source facts")))return false;
 if(PriorResult->HasField(TEXT("checkpoint_file")))
 {
  const auto File=S47J::Obj(PriorResult,TEXT("checkpoint_file"));
  if(!Check(File&&Full(S47J::Str(File,TEXT("path")))==Input&&S47J::Num(File,TEXT("bytes"),-1)==IFileManager::Get().FileSize(*Input)
   &&S47J::Str(File,TEXT("sha1")).Equals(InputSHA,ESearchCase::IgnoreCase),TEXT("when supplied, SC04 checkpoint_file binds exact actual input path/bytes/SHA1")))return false;
 }
 if(!Check(S47J::Int(Original,TEXT("timelineVersion"))==2&&S47J::Str(Original,TEXT("checkpoint"))==TEXT("nightService")&&S47J::Str(Original,TEXT("area"))==TEXT("saro")
  &&(S47J::Int(Spine,TEXT("v"))==1||S47J::Int(Spine,TEXT("v"))==2)&&S47J::Int(Spine,TEXT("sc04"))==2&&S47J::Int(Facts,TEXT("step"))==4&&S47J::Int(Facts,TEXT("sourcesVersion"))==1
  &&S47J::Bool(Facts,TEXT("schedule"))&&S47J::Bool(Facts,TEXT("strip"))&&S47J::Bool(Facts,TEXT("journal"))&&S47J::Bool(Facts,TEXT("binder"))&&S47J::Bool(Facts,TEXT("escaped"))
  &&S47J::Str(Routine,TEXT("phase"))==TEXT("residual")&&S47J::Bool(Routine,TEXT("residualVisible"))&&S47J::Bool(Routine,TEXT("powered"))&&!S47J::Bool(Routine,TEXT("k3"))
  &&S47J::Num(Routine,TEXT("clock"))>=11700&&S47J::Num(Routine,TEXT("clock"))<11760,TEXT("normal actual03:15 SC04 completed-source cohort, passive residual with restored supply")))return false;
 const auto Call=S47J::Obj(Spine,TEXT("sc05d"));
 if(!Check(!Call||(!S47J::Bool(Call,TEXT("done"))&&!S47J::Bool(Call,TEXT("room"))&&!S47J::Bool(Call,TEXT("lineDead"))&&!S47J::Bool(Call,TEXT("compared"))&&!S47J::Bool(Call,TEXT("encoderRead"))),TEXT("SC04 input contains no preplayed SC05 room/comparison/escape shortcut")))return false;
 for(const auto& V:S47J::Arr(Routine,TEXT("docs")))
 {
  if(!Check(V&&V->Type==EJson::Object,TEXT("actual prior document has object payload")))return false;
  const auto D=CanonicalDoc(V->AsObject());const FString Id=S47J::Str(D,TEXT("id"));
  if(!Check(!Id.IsEmpty()&&!Documents.Contains(Id),TEXT("unique actual SC04 input document: ")+Id))return false;Documents.Add(Id,D);
 }
 TSet<FString> Owned;
 for(const auto& V:S47J::Arr(Facts,TEXT("sources")))
 {
  if(!Check(V&&V->Type==EJson::Object,TEXT("owned SC04 source is actual serialized document")))return false;
  const auto D=CanonicalDoc(V->AsObject());const FString Id=S47J::Str(D,TEXT("id"));
  if(!Check(Documents.Contains(Id)&&Same(D,Documents[Id])&&!Owned.Contains(Id),TEXT("owned SC04 source equals actual acquired full document bytes: ")+Id))return false;Owned.Add(Id);
 }
 if(!Check(Owned.Contains(TEXT("sc04_schedule"))&&Owned.Contains(TEXT("sc04_strip"))&&Documents.Num()>=4,TEXT("held schedule and strip plus actual inherited prior documents, never test-refiled")))return false;
 for(const auto& V:S47J::Arr(Routine,TEXT("pointing")))
 {
  if(!Check(V&&V->Type==EJson::Array&&V->AsArray().Num()==2,TEXT("actual source dish encoder pair")))return false;
  const auto& P=V->AsArray();double Az=0,El=0;if(!Check(P[0]->TryGetNumber(Az)&&P[1]->TryGetNumber(El)&&FMath::IsFinite(Az)&&FMath::IsFinite(El),TEXT("finite actual saved encoder values")))return false;InitialPointing.Add({Az,El});
 }
 if(!Check(InitialPointing.Num()==27&&FMath::Abs(InitialPointing[2][0]-42.)<.05,TEXT("actual saved S-03 held042 reference, all27 source pointing values present")))return false;
 // Pure production admission predicate: delete custody bytes only in a local negative copy.
 // Neither comparison mutates the real owner, phase, earlier facts or documents.
 auto MissingSources=S47J::Clone(Facts);MissingSources->RemoveField(TEXT("sources"));MissingSources->RemoveField(TEXT("sourcesVersion"));
 if(!Check(FS47SC05::CanAdoptSC04(Facts)&&!FS47SC05::CanAdoptSC04(MissingSources)&&!FS47SC05::CanAdoptSC04(FJ()),TEXT("actual completed source cohort admits; old Spine v1/gate2 without measured custody does not")))return false;
 return Check(Copy(Input,Root/TEXT("input-sc04.byteoriginal.disk.json"))&&Copy(InputResult,Root/TEXT("input-sc04.byteoriginal.result.json")),TEXT("retain byteoriginal SC04 checkpoint/result and unchanged source hashes"));
}

bool FS47SC05Contract::Begin()
{
 FString Profile,Mode;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Profile);FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);
 Profile=Full(Profile);const FString Saved=Full(FPaths::ProjectSavedDir()),Store=Full(M()->Flow->Saves.Dir);
 // This precedes MakeDirectory, input copying, save writing and any case-state mutation.
 if(!Check(Mode==TEXT("sc05")&&Profile.StartsWith(TEXT("/tmp/"))&&Profile!=TEXT("/tmp")&&FPaths::IsUnderDirectory(Saved,Profile)&&FPaths::IsUnderDirectory(Store,Profile)
  &&M()->Flow->CaseId==0&&M()->Flow->bDevRun&&M()->Flow->Saves.List().IsEmpty()&&!IFileManager::Get().DirectoryExists(*(Saved/TEXT("SC05Contract"))),TEXT("fresh explicit/tmp profile, Saved/Saves containment before every proof/save write")))return false;
 double OuterSeconds=0;FParse::Value(FCommandLine::Get(),TEXT("S47Seconds="),OuterSeconds);
 if(!Check(!FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest"))||(FMath::IsFinite(OuterSeconds)&&OuterSeconds>=480),TEXT("optional SmokeTest only suppresses lost-focus pause and supplies outer timer>=480s; cannot grant contract PASS")))return false;
 for(const auto* Flag:{TEXT("S47NightSequenceContract"),TEXT("S47HorrorContract"),TEXT("S47SubstationTest"),TEXT("S47SubstationTestLoad"),TEXT("S47ProductionViews"),TEXT("S47MotelV2Route"),TEXT("S47Ch4Drive"),TEXT("S47Ch1Drive"),TEXT("S47Ch2Drive"),TEXT("S47SC09Contract"),TEXT("S47RadioWomanContract"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47Auto"),TEXT("S47Face"),TEXT("S47WalkFrom"),TEXT("S47Shot")})
 {FString V;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),V))return Check(false,FString(TEXT("another input/pose/clock runtime driver enabled: "))+Flag);}
 Root=Saved/TEXT("SC05Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*Root,true)){Finish(false,TEXT("own fresh evidence directory unavailable"));return false;}
 Started=true;if(!InputGate())return false;
 const auto Pose=S47J::Obj(Original,TEXT("pose"));if(!Check(Pose&&M()->Player->Walkable(S47J::Num(Pose,TEXT("x")),S47J::Num(Pose,TEXT("z"))),TEXT("actual saved SC04 control-room pose is walkable before load; no test Place")))return false;
 Expected=S47J::Clone(Original);FS47SaveCard Card;Card.Chapter=TEXT("SC04 actual completed custody");Card.Clock=S47Text::ClockText(S47J::Num(S47J::Obj(Original,TEXT("routine")),TEXT("clock")),true);Card.Place=TEXT("SARO control room");FString Error;
 const auto Meta=M()->Flow->Saves.Save(1,TEXT("manual"),0,Expected,Card,Error);FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();
 if(!Check(Meta&&Same(Reader.Load(Meta->Id),Expected),TEXT("seed only actual SC04 full state into isolated real case1; independent disk reader: ")+Error))return false;
 ++Stores;M()->Flow->ToTitle();if(!Check(M()->Mode==US47Main::EMode::Title,TEXT("ordinary title before loading actual SC04 checkpoint")))return false;
 M()->Flow->OpenSave(*Meta);++TitleContinues;NormalContinue=true;
 if(!Restored(TEXT("actual-sc04-input"))||!Others(TEXT("actual-sc04-input"),true)||!Snapshot(TEXT("actual-sc04-input")))return false;
 Began=FPlatformTime::Seconds();LastWalk=M()->Player->Pos;return true;
}

void FS47SC05Contract::Tick(double Dt)
{
 const auto Keep=AsShared();if(Done||!M()||!M()->Flow||!M()->Player||!M()->Story||!M()->Spine||!M()->Evening||!M()->Web||!M()->World||!M()->Inter||!M()->Hud||!M()->UI||!M()->Substation)return;
 if(!Started)
 {
  if(S47Startup::Blocking()||M()->Flow->bStarting||M()->World->bBusy||M()->Mode!=US47Main::EMode::Play)return;
  if(Begin())Next(1,.4);return;
 }
 const double Now=FPlatformTime::Seconds();if(Now-Began>360){Finish(false,TEXT("360s physical SC05 timeout; no clock/pose recovery"));return;}
 if(M()->Flow->bStarting||M()->World->bBusy||S47Startup::Blocking())return;if(Wait>0){Wait-=Dt;return;}
 switch(Stage)
 {
 case 1:
  if(!Check(A()->Active()&&!A()->Done()&&!A()->RoomCompared()&&!A()->EncoderRead()&&M()->Story->Phase()==ES47Phase::Residual&&M()->Story->bResidualVisible&&M()->Story->Rx.Stage==2
   &&M()->Substation->HasNormalResult()&&!M()->Evening->FeederOutage(),TEXT("actual post-SC04 passive residual, normal T01 supply, no SC05 completion fixture")))return;
  if(Go(TEXT("clock")))Next(2);return;
 case 2:if(Walk()&&Face(TEXT("clock")))Next(3,.3);return;
 case 3:
  if(!Use(TEXT("clock"))||!Check(!A()->RoomCompared()&&!A()->Done(),TEXT("early actual clock use grants no room comparison or completion")))return;
  if(Go(TEXT("crtLeft")))Next(4);return;
 case 4:if(Walk()&&Face(TEXT("crtLeft")))Next(5,.3);return;
 case 5:
  if(!Use(TEXT("crtLeft"))||!Check(!A()->EncoderRead()&&!A()->Done(),TEXT("early actual status use cannot grant S-03 event evidence or completion"))||!Sources(TEXT("early negatives"))||!Others(TEXT("early negatives"),true))return;
  if(Go(TEXT("crtCenter")))Next(6);return;
 case 6:if(Walk()&&Face(TEXT("crtCenter")))Next(7,.3);return;
 case 7:if(Use(TEXT("crtCenter")))Next(8,.8);return;
 case 8:if(Console(false))Next(9,.8);return;
 case 9:
  if(M()->ModalOpen())return;if(Cycle(TEXT("before-residual-lock")))Next(10,.4);return;
 case 10:if(Go(TEXT("crtCenter")))Next(11);return;
 case 11:if(Walk()&&Face(TEXT("crtCenter")))Next(12,.3);return;
 case 12:if(Use(TEXT("crtCenter")))Next(13,.8);return;
 case 13:if(Console(true))Next(14,.8);return;
 case 14:
  if(M()->ModalOpen())return;if(Cycle(TEXT("locked-before-solve"))&&Go(TEXT("crtRight")))Next(15);return;
 case 15:if(Walk()&&Face(TEXT("crtRight")))Next(16,.3);return;
 case 16:
  if(!Use(TEXT("crtRight"))||!Check(M()->Story->Phase()==ES47Phase::Solving&&!M()->Story->Rx.bSolved,TEXT("actual tracking terminal starts ordinary direction-solve transaction")))return;
  Next(17);return;
 case 17:
 {
  if(M()->Story->Phase()!=ES47Phase::Printed){if(Now-Mark>20)Finish(false,TEXT("ordinary solve/printer transaction stalled"));return;}
  const auto Paper=M()->Web->Obj(TEXT("room.printerPaper"));
  if(!Check(M()->Story->Rx.bSolved&&M()->Story->Night().SolveAt&&Paper&&Paper.Shown()&&Paper.Scale().Y>=.999,TEXT("actual solve timestamp and fully emerged original printer sheet"))||!Others(TEXT("actual printed sheet"),true))return;
  SolveAt=M()->Story->Night().SolveAt.GetValue();if(Cycle(TEXT("printed-before-tear")))Next(18,.4);return;
 }
 case 18:if(Go(TEXT("printer")))Next(19);return;
 case 19:if(Walk()&&Face(TEXT("printer")))Next(20,.3);return;
 case 20:
 {
  if(!Use(TEXT("printer")))return;
  const auto* D=M()->Story->Docs().FindByPredicate([](const FS47Doc& Doc){return Doc.Id==TEXT("printout");});const auto& Ns=M()->Story->Night();
  if(!Check(M()->Story->Phase()==ES47Phase::Ringing&&D&&D->Page.Contains(TEXT("1420.405"))&&Ns.PrintoutAt&&Ns.SolveAt&&Ns.SolveAt.GetValue()==SolveAt&&M()->ModalOpen(),TEXT("ordinary tear acquires actual complete solve paper and original acquisition/solve stamps")))return;
  AcquiredAt=Ns.PrintoutAt.GetValue();if(!Sources(TEXT("actual acquired solve print"),true)||!BlockedSave(TEXT("Ringing"))||!Snapshot(TEXT("actual-acquired-solve-print")))return;
  Next(21);return;
 }
 case 21:
  if(!Shot(TEXT("solve-paper")))return;M()->UI->Close(true);if(Go(TEXT("phone")))Next(22);return;
 case 22:if(Walk()&&Face(TEXT("phone")))Next(23,.3);return;
 case 23:
  if(!M()->Story->bRinging){if(Now-Mark>12)Finish(false,TEXT("ordinary phone never began ringing"));return;}
  if(!Use(TEXT("phone"))||!Check(M()->Story->Phase()==ES47Phase::Call&&M()->Story->bAnswered&&!M()->Story->bRinging&&M()->Story->IsCinematic()&&M()->Audio&&M()->Audio->Ctx&&M()->Audio->Ctx->State()==TEXT("running"),TEXT("actual phone OnUse enters ordinary FutureCall audio route and cinematic; no Event shortcut"))||!BlockedSave(TEXT("Call"))||!Snapshot(TEXT("actual-future-call")))return;
  Next(24);return;
 case 24:
  if(M()->Story->Phase()!=ES47Phase::Countdown){if(Now-Mark>12)Finish(false,TEXT("ordinary future-call line did not die"));return;}
  CountdownWall=Now;CountdownGt=M()->Story->LineDeadT;
  if(!Check(CountdownGt>=0&&S47J::Bool(A()->Save(),TEXT("lineDead"))&&!S47J::Bool(A()->Save(),TEXT("room"))&&!A()->Done(),TEXT("real line-dead callback begins measured47game-second interval"))||!BlockedSave(TEXT("Countdown"))||!Sources(TEXT("line dead"))||!Snapshot(TEXT("line-dead")))return;
  Next(25);return;
 case 25:
  if(M()->Story->Gt()-CountdownGt<8)return;
  if(!Check(M()->Story->Phase()==ES47Phase::Countdown&&!A()->RoomCompared()&&!A()->EncoderRead()&&!A()->Done(),TEXT("real early countdown cannot finish before actual room event")))return;
  PauseBefore=Measurement();M()->Flow->OpenPause();Next(26);return;
 case 26:
  if(Now-Mark<5)return;PauseWall=Now-Mark;
  if(!Check(M()->Story->IsPaused()&&M()->bPausedByMenu&&Same(PauseBefore,Measurement())&&PauseWall>=5,TEXT("ordinary pause freezes actual clock/gameTime/phase/native encoders for five wall seconds"))||!Snapshot(TEXT("paused-five-wall-seconds")))return;
  M()->UI->Close();DidPause=true;Next(27);return;
 case 27:
  if(!Check(!M()->Story->IsPaused()&&!M()->bPausedByMenu,TEXT("ordinary modal close resumes real story clock")))return;
  if(!S47J::Bool(A()->Save(),TEXT("room"))){if(M()->Story->Gt()-CountdownGt>49)Finish(false,TEXT("47-second room event absent or rejected"));return;}
  if(EventWall==0)EventWall=Now;
  if(!Check(A()->ActualInterval()>=46.99&&A()->ActualInterval()<47.5&&M()->Story->EventClockSec()==S47J::Num(A()->Save(),TEXT("roomAt"))&&DidPause&&EventWall-CountdownWall>=51.5,TEXT("actual room callback follows47game seconds plus real pause walltime, no clock/event setter")))return;
  if(!A()->Shattered()||M()->Web->IsMugVisible()){if(M()->Story->Gt()-CountdownGt>54)Finish(false,TEXT("actual Web/HUD ceramic shatter hook did not complete"));return;}
  if(!Check(A()->Shattered()&&!M()->Web->IsMugVisible()&&!A()->RoomCompared()&&!A()->Done(),TEXT("actual Web mug fall and HUD shatter callback precede comparison; no flag fixture"))||!Others(TEXT("actual room impact"))||!Snapshot(TEXT("actual-room-impact")))return;
  if(Go(TEXT("clock")))Next(28);return;
 case 28:if(Walk()&&Face(TEXT("clock")))Next(29,.3);return;
 case 29:
  if(!Use(TEXT("clock"))||!Check(A()->RoomCompared()&&!A()->EncoderRead()&&!A()->Done()&&Modal(TEXT("MEASURED INTERVAL")),TEXT("ordinary clock OnUse opens actual room/dead-line comparison, still gated by S-03"))||!Sources(TEXT("actual room comparison"),true))return;
  Next(30);return;
 case 30:
  if(!Shot(TEXT("room-clock-comparison")))return;M()->UI->Close(true);if(Go(TEXT("crtLeft")))Next(31);return;
 case 31:if(Walk()&&Face(TEXT("crtLeft")))Next(32,.3);return;
 case 32:
 {
  if(FMath::Abs(M()->Web->DishAz(2)-26.)>.05||FMath::Abs(M()->Web->DishEl(2)-32.)>.05){if(Now-Mark>15)Finish(false,TEXT("actual S-03 encoder failed to settle at026/32"));return;}
  if(!Use(TEXT("crtLeft")))return;
  const auto* D=M()->Story->Docs().FindByPredicate([](const FS47Doc& Doc){return Doc.Id==TEXT("sc05_s03");});
  if(!Check(A()->EncoderRead()&&A()->RoomCompared()&&!A()->Done()&&D&&D->Page.Contains(TEXT("042"))&&D->Page.Contains(TEXT("026.00"))&&D->Page.Contains(TEXT("AUTO SCHEDULE: HELD"))&&Modal(TEXT("CONTROL COMMAND FOR THIS MOTION: NONE"))
   &&M()->Story->Clock()<12300,TEXT("actual single S-03 encoder026/32 versus held042, no command, before03:25/chapter1"))||!Others(TEXT("single antenna record"))||!Sources(TEXT("single antenna record"),true)||!Snapshot(TEXT("all-comparisons-before-transition")))return;
  M()->UI->Close(true);Next(33);return;
 }
 case 33:
  if(M()->Story->ChapterKey!=TEXT("ch1")||M()->Story->IsCinematic()||M()->ModalOpen()){if(Now-Mark>25)Finish(false,TEXT("ordinary completed SC05 chapter card did not reach chapter1"));return;}
  if(!Check(A()->Done()&&A()->RoomCompared()&&A()->EncoderRead()&&A()->Shattered()&&M()->Story->Clock()>=12300&&M()->Story->Clock()<12330,TEXT("actual all-controls completion advances once to03:25/chapter1"))||!Sources(TEXT("after actual chapter transition"))||!Others(TEXT("after actual chapter transition")))return;
  if(Cycle(TEXT("sc05-complete"),true))Next(34,.5);return;
 case 34:
 {
  const auto Case=S47J::Obj(M()->Flow->GameState(),TEXT("case")),Ns=S47J::Obj(Case,TEXT("ns"));
  if(!Check(M()->Story->ChapterKey==TEXT("ch1")&&A()->Done()&&S47J::Num(Ns,TEXT("printoutAt"),-1)==AcquiredAt&&S47J::Num(Ns,TEXT("solveAt"),-1)==SolveAt,TEXT("post-SC05 real Continue retains same original print/solve stamps and completed route"))||!Sources(TEXT("final normal Continue"))||!Others(TEXT("final normal Continue")))return;
  if(!Shot(TEXT("after-normal-continue")))return;
  if(!Check(Stores==5&&TitleContinues==5&&Restores.Num()==5&&Uses==9&&Photos.Num()==3&&Blocked.Num()==3&&DidPause&&PauseWall>=5&&Walked>5&&WalkSamples>50&&NormalContinue,TEXT("all source/dial/ray/47s/ceramic/encoder/disk/title/PNG steps actually executed")))return;
  Finish(true,TEXT("SC05 actual residual solve, original phone,47second room/ceramic control, single S-03 record and full case1 disk/title continuation"));return;
 }
 default:Finish(false,TEXT("unknown SC05 contract stage"));return;
 }
}

void S47SC05Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC05Contract> Run;
 if(Owner.Get()!=Main){Owner=Main;Run=MakeShared<FS47SC05Contract>(Main);}const auto Keep=Run;if(Keep)Keep->Tick(Dt);
}

