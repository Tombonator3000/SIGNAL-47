#include "S47SC11Contract.h"
#include "S47SC11.h"
#include "S47Chapter4.h"
#include "S47Chapter5.h"
#include "S47Docs.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47NightContract.h"
#include "S47SC05.h"
#include "S47Seen.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47ThreatNavigation.h"
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
#include "HAL/PlatformProcess.h"
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
 using FJ=S47J::FObj;
 FString SC11Full(FString Path){Path=FPaths::ConvertRelativePathToFull(Path);FPaths::CollapseRelativeDirectories(Path);FPaths::NormalizeFilename(Path);return Path;}
 bool SC11Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 FString SC11Hash(const FString& Path)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return {};
  FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);return Hash.ToString();
 }
 FJ SC11File(const FString& Path)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("path"),SC11Full(Path));S47J::Set(O,TEXT("bytes"),double(IFileManager::Get().FileSize(*Path)));S47J::Set(O,TEXT("sha1"),SC11Hash(Path));return O;
 }
 FJ SC11Read(const FString& Path){FString Text;return FFileHelper::LoadFileToString(Text,*Path)?S47J::Read(Text):FJ();}
 FJ SC11SourceDoc(const FJ& Saved)
 {
  if(!Saved)return nullptr;auto O=S47J::Clone(Saved);if(!O)return nullptr;
  // The two existing production FS47Doc serializers differ only in this field name.
  if(S47J::Has(O,TEXT("paperRatio"))){if(S47J::Has(O,TEXT("ratio"))&&S47J::Num(O,TEXT("ratio"))!=S47J::Num(O,TEXT("paperRatio")))return nullptr;S47J::Set(O,TEXT("ratio"),S47J::Num(O,TEXT("paperRatio")));O->RemoveField(TEXT("paperRatio"));}
  return O;
 }
 FJ SC11Pointing(US47Main* M)
 {
  auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> A;
  for(int32 I=0;I<M->Web->DishCount();++I)A.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(M->Web->DishAz(I)),MakeShared<FJsonValueNumber>(M->Web->DishEl(I))}));
  O->SetArrayField(TEXT("pointing"),A);return O;
 }
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

bool FS47SC11Contract::StableInput(const FJ& Saved,const FString& Context)
{
 const auto Now=M->Flow->GameState(),Pose=S47J::Obj(Saved,TEXT("pose")),Case=S47J::Obj(Saved,TEXT("case"));
 const auto WasSpine=S47J::Obj(Saved,TEXT("spine")),NowSpine=S47J::Obj(Now,TEXT("spine"));
 if(!Check(M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->Story->bEveningTimeline
  &&Pose&&M->Player->Walkable(M->Player->Pos.X,M->Player->Pos.Z)&&M->World->Area==S47J::Str(Saved,TEXT("area"))
  &&M->Flow->CheckpointName()==S47J::Str(Saved,TEXT("checkpoint"))&&M->Story->Clock()==S47J::Num(Case,TEXT("clock"))
  &&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.0001&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.0001
  &&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.00001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.00001,
  Context+TEXT(" actual case1/title Continue retains exact clock/area/checkpoint/walkable pose without Place")))return false;
 const auto Match=[&](const FJ& A,const FJ& B,const TCHAR* Key)
 {const auto X=A?A->TryGetField(Key):nullptr,Y=B?B->TryGetField(Key):nullptr;return(!X&&!Y)||(X&&Y&&FJsonValue::CompareEqual(*X,*Y));};
 for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4"),TEXT("ch5")})
  if(!Check(Match(Case,S47J::Obj(Now,TEXT("case")),Key),Context+TEXT(" full original chapter bytes: ")+Key))return false;
 for(const TCHAR* Key:{TEXT("sc04"),TEXT("sc08"),TEXT("sc11"),TEXT("sc12Route"),TEXT("sc04d"),TEXT("sc05d"),TEXT("sc06d"),TEXT("sc07d"),TEXT("sc08d"),TEXT("sc11d")})
  if(!Check(Match(WasSpine,NowSpine,Key),Context+TEXT(" actual prior gate/source payload: ")+Key))return false;
 const int32 Version=S47J::Int(NowSpine,TEXT("v"));
 if(!Check((Version==1||Version==2)&&M->Web->DishCount()==27,Context+TEXT(" supported source-qualified v1/v2 and actual27 encoders")))return false;
 for(const TCHAR* Key:{TEXT("doors"),TEXT("horror"),TEXT("observing"),TEXT("substation")})
  if(!Check(Match(Saved,Now,Key),Context+TEXT(" preserved production field: ")+Key))return false;
 return Check(FFileHelper::SaveStringToFile(S47J::Write(Now,true),*(Root/(Context+TEXT(".actual.json")))),Context+TEXT(" retain full unmodified post-Continue state"));
}

bool FS47SC11Contract::AdoptInput()
{
 InputPath=SC11Full(InputPath);const FString ResultPath=FPaths::GetPath(InputPath)/TEXT("result.json");
 InputState=SC11Read(InputPath);const auto Prior=SC11Read(ResultPath),Pin=S47J::Obj(Prior,TEXT("checkpoint_file"));
 InputFile=SC11File(InputPath);InputResultFile=SC11File(ResultPath);Chain=S47J::Str(Prior,TEXT("chain_id"));
 if(!Check(FPaths::GetCleanFilename(InputPath)==TEXT("sc10-complete.disk.json")&&InputState&&Prior&&Pin
  &&S47J::Str(Prior,TEXT("status"))==TEXT("PASS")&&S47J::Str(Prior,TEXT("scenario"))==TEXT("SC10")
  &&S47J::Str(Prior,TEXT("mode"))==TEXT("actual-checkpoint")&&!Chain.IsEmpty()
  &&SC11Full(S47J::Str(Pin,TEXT("path")))==InputPath&&S47J::Num(Pin,TEXT("bytes"))==S47J::Num(InputFile,TEXT("bytes"))
  &&S47J::Num(InputFile,TEXT("bytes"))>0&&S47J::Str(Pin,TEXT("sha1")).Equals(S47J::Str(InputFile,TEXT("sha1")),ESearchCase::IgnoreCase),
  TEXT("actual SC10 complete disk is byte/hash-bound to its sibling PASS/scenario/actual mode/nonempty chain_id")))return false;
 const auto Spine=S47J::Obj(InputState,TEXT("spine")),Case=S47J::Obj(InputState,TEXT("case")),C4=S47J::Obj(Case,TEXT("ch4")),Siege=S47J::Obj(Spine,TEXT("sc11d"));
 const int32 Version=S47J::Int(Spine,TEXT("v"));
 if(!Check(S47J::Int(InputState,TEXT("timelineVersion"))==2&&(Version==1||Version==2)&&FS47SC05::CanAdoptSC04(S47J::Obj(Spine,TEXT("sc04d")))
  &&S47J::Int(Spine,TEXT("sc04"))==2&&S47J::Int(Spine,TEXT("sc08"))==2&&S47J::Int(Spine,TEXT("sc11"))==1
  &&S47J::Int(Spine,TEXT("sc12Route"))==1&&Siege&&S47J::Int(Siege,TEXT("step"))==int32(FS47SC11::EStep::Return)
  &&!S47J::Bool(Siege,TEXT("ctx_hr_h07_escape_complete"))&&S47J::Str(C4,TEXT("stage"))==TEXT("complete")
  &&S47J::Bool(C4,TEXT("told"))&&S47J::Bool(C4,TEXT("p10"))&&S47J::Bool(C4,TEXT("p11"))&&S47J::Bool(C4,TEXT("p12"))
  &&!S47J::Obj(Case,TEXT("ch5"))&&S47J::Str(InputState,TEXT("area"))==TEXT("saro")&&S47J::Str(InputState,TEXT("checkpoint"))==TEXT("chapter4")
  &&S47J::Num(Case,TEXT("clock"))>=FS47SC11::Arrive,TEXT("real C4 complete at05:00 or later, retained SC04 sources, SC11 Return at motel before actual SARO arrival; legacy fixture not adopted")))return false;
 for(const TCHAR* Key:{TEXT("receiver"),TEXT("event"),TEXT("hondo"),TEXT("site11"),TEXT("checkedLocalLight"),TEXT("observedRoom"),TEXT("ctx_hr_h07_escape_complete")})
  if(!Check(!S47J::Bool(Siege,Key),FString(TEXT("pending SC10 input has no unplayed SC11 fact: "))+Key))return false;
 TArray<uint8> DiskBytes,ResultBytes;
 if(!Check(FFileHelper::LoadFileToArray(DiskBytes,*InputPath)&&FFileHelper::LoadFileToArray(ResultBytes,*ResultPath)
  &&FFileHelper::SaveArrayToFile(DiskBytes,*(Root/TEXT("sc10-input-original.disk.json")))&&FFileHelper::SaveArrayToFile(ResultBytes,*(Root/TEXT("sc10-input-result-original.json")))
  &&SC11Hash(Root/TEXT("sc10-input-original.disk.json"))==SC11Hash(InputPath)&&SC11Hash(Root/TEXT("sc10-input-result-original.json"))==SC11Hash(ResultPath),TEXT("retain byte-original SC10 input and raw PASS privately, never rewrite their source files")))return false;
 // Import the complete foreign checkpoint into this fresh isolated case; no field is patched.
 FS47SaveCard Card;Card.Chapter=TEXT("SC10 actual input");Card.Place=TEXT("Sierra Motor Court");Card.Clock=TEXT("actual saved time");FString Error;
 const auto Meta=M->Flow->Saves.Save(1,TEXT("manual"),0,InputState,Card,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();
 if(!Check(Meta&&SC11Same(Reader.Load(Meta->Id),InputState),TEXT("isolated case1 storage and independent reader preserve full SC10 state: ")+Error))return false;
 ++Stores;M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),Live=M->Flow->Saves.ContinueSave();
 if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&Live&&Selected->Id==Meta->Id&&Live->Id==Meta->Id&&SC11Same(TitleReader.Load(Selected->Id),InputState)
  &&M->Flow->OpenTestView(TEXT("continue")),TEXT("actual input enters via ordinary title/fresh reader/Continue, not fixture StartGame")))return false;
 if(!StableInput(InputState,TEXT("sc10-input")))return false;
 if(!Check(Act()&&Act()->Phase()==FS47SC11::EStep::Return&&!M->World->bDriving&&!M->World->bCommuteVehicle&&M->World->TruckAt==TEXT("motel")
  &&M->World->AtMotel(M->Player->Pos)&&!M->Story->Begun.Contains(TEXT("ch5"))
  &&M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("obs_runbinder")&&!D.Page.IsEmpty();}),TEXT("actual motel return position/service truck and genuine20:30 binder; no OnEnd, prior-doc factory or transport fixture")))return false;
 const auto Array=S47J::Obj(Spine,TEXT("sc08d"));InitialPointing=S47J::New();InitialPointing->SetArrayField(TEXT("pointing"),S47J::Arr(Array,TEXT("pointings")));
 if(!Check(S47J::Int(Array,TEXT("v"))==1&&S47J::Bool(Array,TEXT("escaped"))&&S47J::Int(Array,TEXT("plays"))>=2
  &&S47J::Arr(Array,TEXT("pointings")).Num()==27&&SC11Same(InitialPointing,SC11Pointing(M)),TEXT("all27 loaded encoders equal actual completed SC08 serialized readings, not a new post-load baseline")))return false;
 const auto SavedSources=S47J::Arr(Array,TEXT("sources")),LiveDocs=S47J::Arr(M->Story->SaveRoutine(),TEXT("docs"));
 if(!Check(!SavedSources.IsEmpty(),TEXT("completed SC08 carries the actual preceding source inventory")))return false;
 for(const auto& V:SavedSources)
 {
  const auto Paper=V&&V->Type==EJson::Object?SC11SourceDoc(V->AsObject()):FJ();
  if(!Check(Paper&&!S47J::Str(Paper,TEXT("id")).IsEmpty()&&LiveDocs.ContainsByPredicate([&](const TSharedPtr<FJsonValue>& D){return D&&D->Type==EJson::Object&&SC11Same(Paper,D->AsObject());}),TEXT("input full serialized source/sheets match actual owned document: ")+S47J::Str(Paper,TEXT("id"))))return false;
 }
 for(const FS47Doc& Doc:M->Story->Docs())OriginalDocs.Add(Doc.Id,Doc.Page);
 for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object)FullOriginalDocs.Add(S47J::Str(V->AsObject(),TEXT("id")),S47J::Clone(V->AsObject()));
 auto R=S47J::New();S47J::Set(R,TEXT("name"),TEXT("actual-sc10-title-continue"));S47J::Set(R,TEXT("pass"),true);S47J::Set(R,TEXT("checkpoint"),InputFile);S47J::Set(R,TEXT("result"),InputResultFile);Restores.Add(MakeShared<FJsonValueObject>(R));
 NormalContinue=true;return true;
}

bool FS47SC11Contract::GoItem(const FString& Id)
{
 const auto* It=M->Inter->Get(Id);const auto* WebItem=M->Web->Item(Id);const FS47Obj Object=It&&It->Object?It->Object:WebItem?M->Web->ObjAt(WebItem->Node):FS47Obj();
 const FBox B=Object.WorldBox();if(!Check(Object&&B.IsValid,TEXT("actual approach target: ")+Id))return false;
 const double Range=It&&It->Range>0?It->Range:WebItem?WebItem->Range:3.2;const FVector Center=B.GetCenter();const FVector2D From(M->Player->Pos.X,M->Player->Pos.Z);
 TArray<FVector2D> Seeds;for(const auto& N:FS47SC11::Nodes())Seeds.Add(N.At);
 const auto Segment=[&](const FVector2D& A,const FVector2D& Z){const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,Z)/.05));for(int32 I=0;I<=N;++I){const auto P=FMath::Lerp(A,Z,double(I)/N);if(!M->Player->Walkable(P.X,P.Y))return false;}return true;};
 TArray<FVector2D> Best;double Cost=1e100;
 for(double R:{1.1,1.6,2.1,2.6})for(int32 I=0;I<16;++I)
 {
  const double Angle=UE_DOUBLE_PI*I/8;const FVector2D To(Center.X+R*FMath::Cos(Angle),Center.Z+R*FMath::Sin(Angle));if(!M->Player->Walkable(To.X,To.Y))continue;
  const FVector Eye(To.X,M->Player->FloorY+M->Player->Eye,To.Y),D=Center-Eye;
  if(D.Size()>Range-.08||M->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)continue;
  auto Found=FS47ThreatNavigation::FindPath(From,To,Seeds,M->Web->Colliders(),M->Player->Radius,Segment);double Length=0;auto Previous=From;
  for(const auto& P:Found){Length+=FVector2D::Distance(Previous,P);Previous=P;}if(!Found.IsEmpty()&&Length<Cost){Best=MoveTemp(Found);Cost=Length;}
 }
 if(!Check(!Best.IsEmpty(),TEXT("complete bounded walkable path to actual ray target, no Place recovery: ")+Id))return false;
 Path(MoveTemp(Best));return true;
}

bool FS47SC11Contract::ExportBoundary()
{
 auto* C5=Cast<US47Chapter5>(M->Story->Chapter(TEXT("ch5")));
 if(!Check(C5&&C5->bActive&&C5->RevisedDiner()&&C5->S.Stage==TEXT("to-truck")&&!C5->S.bArrived&&!C5->S.SC12.bLaid
  &&M->Spine->Gate(FS47Spine::SC11)==2&&Act()->F.Escaped&&M->World->Area==TEXT("saro")&&!M->World->bDriving
  &&M->Flow->CheckpointName()==TEXT("chapter5"),TEXT("natural C5 start after actual SC11 escape, before any SC12 action; actual clock retained")))return false;
 if(!Sources(TEXT("sc11-complete")))return false;
 FJ Proof;FString Error;const bool Cycled=S47NightContract::RoundTrip(M,Root,TEXT("sc11-complete"),Proof,Error);
 if(!Check(Cycled,TEXT("SC11 output genuine manual/fresh-reader/title/Continue: ")+Error))return false;
 ++Stores;Restores.Add(MakeShared<FJsonValueObject>(Proof));CheckpointFile=SC11File(Root/TEXT("sc11-complete.disk.json"));
 return Check(SC11Same(InputFile,SC11File(InputPath))&&SC11Same(InputResultFile,SC11File(FPaths::GetPath(InputPath)/TEXT("result.json")))
  &&SC11Same(SC11Pointing(M),InitialPointing),TEXT("original SC10 input/PASS and all27 encoders remain unchanged through the actual SC11 journey"));
}

void FS47SC11Contract::Finish(bool Good,const FString& Why)
{
 const auto KeepAlive=AsShared();
 if(Done)return;Done=true;
 if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 if(M&&M->World)M->World->Autopilot.Reset();
 auto Out=S47J::New();S47J::Set(Out,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(Out,TEXT("mode"),Mode);
 S47J::Set(Out,TEXT("scenario"),TEXT("SC11"));S47J::Set(Out,TEXT("mode"),ActualInput?TEXT("actual-checkpoint"):Mode);S47J::Set(Out,TEXT("chain_id"),Chain);
 S47J::Set(Out,TEXT("process_id"),double(FPlatformProcess::GetCurrentProcessId()));S47J::Set(Out,TEXT("checkpoint_file"),CheckpointFile);
 auto Prior=S47J::New();S47J::Set(Prior,TEXT("checkpoint"),InputFile);S47J::Set(Prior,TEXT("result"),InputResultFile);S47J::Set(Out,TEXT("prerequisite"),Prior);
 S47J::Set(Out,TEXT("driven_metres"),Driven);S47J::Set(Out,TEXT("final_state"),M&&M->Flow?M->Flow->GameState():FJ());Out->SetArrayField(TEXT("transitions"),Transitions);
 S47J::Set(Out,TEXT("stage"),Stage);S47J::Set(Out,TEXT("reason"),Why);S47J::Set(Out,TEXT("checks"),Checks);S47J::Set(Out,TEXT("actual_ray_uses"),Uses);
 S47J::Set(Out,TEXT("stores"),Stores);S47J::Set(Out,TEXT("walked_metres"),Walked);S47J::Set(Out,TEXT("walk_samples"),WalkSamples);
 S47J::Set(Out,TEXT("normal_title_continue"),NormalContinue);S47J::Set(Out,TEXT("actual_body_render_witness"),RenderWitness);
 S47J::Set(Out,TEXT("requested_picture_paths"),Pictures);S47J::Set(Out,TEXT("final_facts"),Act()?Act()->Save():S47J::FObj());
 S47J::Set(Out,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(Out,TEXT("method"),TEXT("Declared chapter4/prior-night and OnEnd fixture; one walkable SARO start. All subsequent player movement is native Walker input/Resolve, all world uses actual camera ray/focus/HUD OnUse. Actual local intensity, three moving spotlights, room observation, physical escape/capture and fresh disk/full Flow/title Continue."));
 S47J::Set(Out,TEXT("limits"),TEXT("SC01-SC10 and the return drive are explicit fixtures, not replayed. Escape is the actual truck-pad approach boundary, not boarding/driving. Closed-door query is a named reversible geometry fixture; opacity/depth flags and LOS are not pixel-based visual acceptance. API input is not human input; PNG integrity is not visual/FPS acceptance."));
 if(ActualInput)
 {
  S47J::Set(Out,TEXT("method"),TEXT("Actual hash-bound SC10 sibling PASS/full case1 checkpoint. Ordinary native Walker to the retained motel truck; labelled native DriveBack/autopilot controls, continuous actual drive and park, then Walker/camera-ray/OnUse into SARO. Production sources/light/observation/hunt/escape and natural C5 start. Genuine manual/fresh-reader/title/Continue at every stable boundary; original full source payloads and27 encoders preserved."));
  S47J::Set(Out,TEXT("limits"),TEXT("Prior scenarios are bound actual predecessor evidence, not replayed by this process. Test route-target/autopilot/panel APIs are not human input. Reversible closed-door geometry query is labelled separately. Body proof is recent native render plus clear camera-to-torso-bounds sight/view angle, not a collision hit or visual/pixel acceptance. Output precedes SC12 actions; full-night, packaging and installation remain unaccepted."));
 }
 Out->SetArrayField(TEXT("executed_checks"),Executed);Out->SetArrayField(TEXT("rays"),Rays);Out->SetArrayField(TEXT("states"),States);
 Out->SetArrayField(TEXT("restores"),Restores);Out->SetArrayField(TEXT("sight_queries"),Sight);Out->SetArrayField(TEXT("photos"),Photos);
 const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(Out,true),*(Root/TEXT("result.json")));
 if(Good&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract %s PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm evidence=%s"),*Mode,Checks,Stores,Uses,Walked,*Root);}
 else {UE_LOG(LogTemp,Error,TEXT("S47: spine contract %s FAIL stage=%d: %s (result written %d)"),*Mode,Stage,*Why,Written);}
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
 if(ActualInput&&!Player->Walkable(Player->Pos.X,Player->Pos.Z)){Finish(false,TEXT("actual predecessor route left native walkable land; no recovery"));return false;}
 Walked+=Distance;++WalkSamples;LastWalk=Player->Pos;
 if(!Route.IsValidIndex(Cursor)){Player->TestMove.Reset();return true;}
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
bool FS47SC11Contract::Use(const FString& Id,bool Activate)
{
 const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Picked=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);
 auto Row=S47J::New();S47J::Set(Row,TEXT("id"),Id);S47J::Set(Row,TEXT("picked"),Picked);S47J::Set(Row,TEXT("focused"),Focus);
 S47J::Set(Row,TEXT("eye_ue_cm"),Eye.ToString());S47J::Set(Row,TEXT("forward_ue"),Forward.ToString());S47J::Set(Row,TEXT("target_distance_m"),FVector::Dist(Eye,Aim)/100.);
 const auto* Item=M->Inter->Get(Id);const auto* WebItem=M->Web->Item(Id);
 const double Range=Item && Item->Range>0?Item->Range:WebItem?WebItem->Range:-1.;
 S47J::Set(Row,TEXT("actual_range_m"),Range);Rays.Add(MakeShared<FJsonValueObject>(Row));
 S47J::Set(Row,TEXT("activate_bound_hud_onuse"),Activate);
 if(!Check(Picked==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary HUD OnUse: ")+Id))return false;
 if(Activate){M->Player->OnUse(Picked);++Uses;}return true;
}
bool FS47SC11Contract::Document(const FString& Id,const FString& Fragment)
{
 const FS47Doc* Doc=M->Story->Docs().FindByPredicate([&](const FS47Doc& Value){return Value.Id==Id;});
 TArray<FString> Text;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)SC11Text(Modal.ToSharedRef(),Text);
 const bool Visible=Text.ContainsByPredicate([&](const FString& Value){return Value.Contains(Fragment);});
 if(!Check(Doc&&Doc->Page.Contains(Fragment)&&M->UI->Modal()&&Visible,TEXT("actual original document and visible modal: ")+Id))return false;
 SourcePages.Add(Id,Doc->Page);
 if(ActualInput)for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object&&S47J::Str(V->AsObject(),TEXT("id"))==Id)FullOriginalDocs.Add(Id,S47J::Clone(V->AsObject()));
 M->UI->Close(true);return true;
}
bool FS47SC11Contract::Sources(const FString& Context)
{
 FS47SC11* A=Act();if(!Check(A!=nullptr,Context+TEXT(" live siege owner")))return false;
 if(ActualInput)
 {
  const auto Docs=S47J::Arr(M->Story->SaveRoutine(),TEXT("docs"));
  for(const auto& Entry:FullOriginalDocs)
   if(!Check(Docs.ContainsByPredicate([&](const TSharedPtr<FJsonValue>& V){return V&&V->Type==EJson::Object&&SC11Same(V->AsObject(),Entry.Value);}),Context+TEXT(" complete actual document fields/sheets preserved: ")+Entry.Key))return false;
  if(!Check(SC11Same(SC11Pointing(M),InitialPointing),Context+TEXT(" all27 actual encoders preserved")))return false;
 }
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
 M->UI->Close(true);const FString Block=ActualInput?M->Flow->SaveBlock():M->Flow->GameSaveBlock();
 if(!Block.IsEmpty()||M->Story->IsCinematic()||M->World->bBusy||M->ModalOpen())
 {if(FPlatformTime::Seconds()-Mark>20)Finish(false,TEXT("ordinary gameplay save gate: ")+Name+TEXT(" / ")+Block);return false;}
 Expected=S47J::Clone(M->Flow->GameState());
 if(ActualInput)
 {
  if(!Sources(TEXT("before manual ")+Name))return false;
  FJ Proof;FString Error;const bool Cycled=S47NightContract::RoundTrip(M,Root,Name,Proof,Error);
  if(!Check(Cycled,TEXT("ordinary actual case1 manual/fresh-reader/title/Continue: ")+Name+TEXT(" / ")+Error))return false;
  ++Stores;Pending=Name;NormalContinue=true;return true;
 }
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
 if(ActualInput)
 {
  const auto Now=M->Flow->GameState();const auto BeforeCase=S47J::Obj(Expected,TEXT("case")),NowCase=S47J::Obj(Now,TEXT("case"));
  for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4"),TEXT("ch5")})
  {
   const auto X=BeforeCase?BeforeCase->TryGetField(Key):nullptr,Y=NowCase?NowCase->TryGetField(Key):nullptr;
   if(!Check((!X&&!Y)||(X&&Y&&FJsonValue::CompareEqual(*X,*Y)),FString(TEXT("actual full Flow retains prior chapter/source state: "))+Key+TEXT(" / ")+Pending))return false;
  }
  const auto NowSpine=S47J::Obj(Now,TEXT("spine"));
  for(const TCHAR* Key:{TEXT("sc04d"),TEXT("sc05d"),TEXT("sc06d"),TEXT("sc07d"),TEXT("sc08d")})
  {
   const auto X=Spine?Spine->TryGetField(Key):nullptr,Y=NowSpine?NowSpine->TryGetField(Key):nullptr;
   if(!Check((!X&&!Y)||(X&&Y&&FJsonValue::CompareEqual(*X,*Y)),FString(TEXT("actual full Flow retains preceding Spine source payload: "))+Key+TEXT(" / ")+Pending))return false;
  }
 }
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
 const auto* Torso=A->Pale.Torso.Get();if(!Check(Torso!=nullptr,TEXT("actual torso component supplies current native bounds")))return false;
 const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const double Dot=FVector::DotProduct(Forward,(Torso->Bounds.Origin-Eye).GetSafeNormal());
 const bool Clear=A->ClearSight(S47Three::ToThree(Eye),S47Three::ToThree(Torso->Bounds.Origin));
 if(!Check(Clear&&Dot>.995&&Torso->WasRecentlyRendered(.25f),TEXT("actual camera clear to native torso bounds, dot>.995 and recent render; collisionless original needs no body physics hit")))return false;
 auto Witness=S47J::New();S47J::Set(Witness,TEXT("method"),TEXT("Actual collisionless torso bounds, camera ClearSight/view dot and recent render; opaque/main-pass/depth checks, no body physics-hit or pixel acceptance"));
 S47J::Set(Witness,TEXT("camera_eye_ue_cm"),Eye.ToString());S47J::Set(Witness,TEXT("camera_forward_ue"),Forward.ToString());S47J::Set(Witness,TEXT("native_torso_component"),Torso->GetPathName());
 S47J::Set(Witness,TEXT("native_torso_bounds_origin_ue_cm"),Torso->Bounds.Origin.ToString());S47J::Set(Witness,TEXT("native_torso_bounds_extent_cm"),Torso->Bounds.BoxExtent.ToString());
 S47J::Set(Witness,TEXT("camera_dot"),Dot);S47J::Set(Witness,TEXT("clear_sight"),Clear);S47J::Set(Witness,TEXT("recent_render"),true);Witness->SetArrayField(TEXT("actual_body_parts"),Parts);Sight.Add(MakeShared<FJsonValueObject>(Witness));
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
  ActualInput=FParse::Value(FCommandLine::Get(),TEXT("S47SC10Checkpoint="),InputPath);
  const bool NoParentTraversal=!UserDir.Contains(TEXT(".."));
  UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
  if(!Check((Mode==TEXT("sc11")||Mode==TEXT("sc11long")||Mode==TEXT("sc11retry"))&&M->Flow->CaseId==0&&M->Flow->bDevRun&&(ActualInput?Jump.IsEmpty():Jump==TEXT("chapter4"))&&NoParentTraversal
   &&(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
   &&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir)
   &&!IFileManager::Get().DirectoryExists(*(FPaths::ProjectSavedDir()/TEXT("SC11Contract"))),TEXT("explicit isolated fresh /tmp profile; actual input has no jump, legacy keeps named chapter4 fixture; Saved/Saves contained before writes")))return;
  Root=FPaths::ProjectSavedDir()/TEXT("SC11Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);Started=true;
  for(const TCHAR* Flag:{TEXT("S47Ch4Drive"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract"),TEXT("S47RadioWomanContract"),TEXT("S47ProductionViews"),TEXT("S47Drive"),TEXT("S47Auto"),TEXT("S47WalkFrom"),TEXT("S47Place"),TEXT("S47EveningBootstrap"),TEXT("S47SC11Checkpoint")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Finish(false,TEXT("independent runtime driver also enabled"));return;}}
  if(!Check(M->Flow->Saves.List().IsEmpty(),TEXT("fresh isolated SaveStore required before input import or fixture")))return;
  if(ActualInput){if(AdoptInput()&&GoItem(TEXT("truck")))Next(100,0);return;}
  if(!Check(M->Spine->Enabled()&&M->Story->bEveningTimeline&&M->World->Area==TEXT("saro")&&Act(),TEXT("fresh isolated new-evening SC11, actual siege available")))return;
  auto* C4=Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));
  if(!Check(C4&&C4->bActive&&C4->OnEnd&&M->Player->Walkable(1.2,.3),TEXT("declared prior SC01-SC10/chapter4 fixture and legal SARO start")))return;
  for(const FS47Doc& Doc:M->Story->Docs())OriginalDocs.Add(Doc.Id,Doc.Page);
  C4->S.Stage=TEXT("complete");C4->S.bP10=C4->S.bP11=C4->S.bP12=C4->S.bTold=true;
  UE_LOG(LogTemp,Display,TEXT("S47: SC11 FIXTURE prior SC01-SC10 supplied by chapter4 jump/OnEnd; not replayed, no return drive claim"));
  M->Player->Place(1.2,.3,0);C4->OnEnd();Next(1,.6);return;
 }
 if(FPlatformTime::Seconds()-Began>(ActualInput?900:300)){Finish(false,TEXT("SC11 contract timeout; actual motel return has a separate explicit900s budget"));return;}
 if(M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy)return;
 if(Hold>0){Hold-=Dt;return;}
 if(!Pending.IsEmpty()&&!Restored())return;
 FS47SC11* A=Act();if(!A){Finish(false,TEXT("siege owner lost"));return;}
 if(Stage>=50&&Stage<80&&A->F.Captures!=CapturesBefore){Finish(false,TEXT("unexpected actual capture on chosen escape route"));return;}
 switch(Stage)
 {
 case 100:if(Walk()){if(Face(TEXT("truck")))Next(101,.35);}return;
 case 101:
  if(!Check(A->Phase()==FS47SC11::EStep::Return&&!A->F.Receiver&&!A->F.Escaped&&M->World->AtMotel(M->Player->Pos)&&M->World->TruckAt==TEXT("motel"),TEXT("motel remains real SC11 Return; no collection or escape before driving"))||!Use(TEXT("truck"),false))return;
  DriveOdo=M->World->Odometer();SawDriving=false;LastDriveLeg.Reset();M->World->Autopilot=10;M->World->DriveBack();Next(102,.2);return;
 case 102:
 {
  if(M->World->bDriving)
  {
   SawDriving=true;const double Distance=(M->World->Odometer()-DriveOdo)*1609.344;
   if(Distance<0||M->World->bCommuteVehicle){Check(false,TEXT("ordinary motel→SARO trip lost service vehicle/continuous odometer"));return;}
   if(LastDriveLeg!=M->World->Leg)
   {
    LastDriveLeg=M->World->Leg;auto R=S47J::New();S47J::Set(R,TEXT("name"),TEXT("actual-return-drive-leg"));S47J::Set(R,TEXT("leg"),LastDriveLeg);S47J::Set(R,TEXT("area"),M->World->Area);
    S47J::Set(R,TEXT("clock"),M->Story->Clock());S47J::Set(R,TEXT("odometer"),M->World->Odometer());S47J::Set(R,TEXT("actual_truck_three_m"),M->World->CabTruck()?M->World->CabTruck()->Position.ToString():TEXT("missing"));Transitions.Add(MakeShared<FJsonValueObject>(R));
   }
   return;
  }
  if(!SawDriving){if(FPlatformTime::Seconds()-Mark>15)Finish(false,TEXT("normal motel return never entered the actual drive"));return;}
  Driven=(M->World->Odometer()-DriveOdo)*1609.344;M->World->Autopilot.Reset();
  auto R=S47J::New();S47J::Set(R,TEXT("name"),TEXT("ordinary-motel-to-saro"));S47J::Set(R,TEXT("driven_metres"),Driven);S47J::Set(R,TEXT("odometer_before"),DriveOdo);S47J::Set(R,TEXT("odometer_after"),M->World->Odometer());S47J::Set(R,TEXT("autopilot_target_mps"),10);S47J::Set(R,TEXT("actual_state"),M->Flow->GameState());Transitions.Add(MakeShared<FJsonValueObject>(R));
  if(!Check(Driven>400&&M->World->Area==TEXT("saro")&&M->World->TruckAt==TEXT("saro")&&M->World->TruckOnLot()&&A->Phase()==FS47SC11::EStep::Return,TEXT("real return drive parked at SARO; outside pad never counts as bounded room arrival")))return;
  if(GoItem(TEXT("doorEast")))Next(103,0);return;
 }
 case 103:if(Walk()){if(Face(TEXT("doorEast")))Next(104,.35);}return;
 case 104:
 {
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Use(TEXT("doorEast")))return;
  if(!Check(M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("ordinary actual east-door use, no collider/visibility setter")))return;
  TArray<FVector2D> Seeds;for(const auto& N:FS47SC11::Nodes())Seeds.Add(N.At);
  const FVector2D From(M->Player->Pos.X,M->Player->Pos.Z),To(1.2,.3);
  auto Entry=FS47ThreatNavigation::FindPath(From,To,Seeds,M->Web->Colliders(),M->Player->Radius,[&](const FVector2D& X,const FVector2D& Y)
  {const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(X,Y)/.05));for(int32 I=0;I<=N;++I){const auto P=FMath::Lerp(X,Y,double(I)/N);if(!M->Player->Walkable(P.X,P.Y))return false;}return true;});
  if(!Check(!Entry.IsEmpty(),TEXT("complete native walkable open-door entry into bounded control room; no pose recovery")))return;
  Path(MoveTemp(Entry));Next(105,0);return;
 }
 case 105:if(Walk()){if(!Check(A->Phase()==FS47SC11::EStep::Collect&&FS47SC11::Room(FVector2D(M->Player->Pos.X,M->Player->Pos.Z))==0,TEXT("only actual Walker entry into SARO control room starts SC11 collection")))return;Next(1,0);}return;
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
  const double ViewDot=FVector::DotProduct(M->Player->ForwardUE(),(Torso->Bounds.Origin-M->Player->EyeUE()).GetSafeNormal());
  if(!A->Pale.Rendered()||!A->ClearSight(Cam.Pos,Target)||ViewDot<=.995||!S47Facing(Cam,Target,20,8))
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
  if(ActualInput)
  {
   if(!ExportBoundary())return;
   for(const FString& File:Pictures){FJ Receipt;if(!Check(SC11Png(File,Receipt),TEXT("complete actual PNG CRC/IEND/EOF/SHA: ")+File))return;Photos.Add(MakeShared<FJsonValueObject>(Receipt));}
   if(!Check(Stores>=6&&Restores.Num()>=6&&RenderWitness&&NormalContinue&&Uses>=8&&Driven>400&&Walked>(Mode==TEXT("sc11long")?80:30),TEXT("actual predecessor/return/collection/hunt/escape and ordinary disk/title chain completed")))return;
   Finish(true,TEXT("SC11 actual SC10→motel return→SARO sources/light/hunt/escape→natural C5 boundary"));return;
  }
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
 const auto KeepAlive=Run;if(KeepAlive)KeepAlive->Tick(Dt);
}
