#include "S47SC07Contract.h"
#include "S47Chapter2.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Office.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
 S47J::FObj SC07File(const FString& Path)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*Path))return nullptr;FSHAHash H;FSHA1::HashBuffer(B.GetData(),B.Num(),H.Hash);
  auto O=S47J::New();S47J::Set(O,TEXT("path"),Path);S47J::Set(O,TEXT("bytes"),B.Num());S47J::Set(O,TEXT("sha1"),H.ToString());return O;
 }
 bool SC07Same(const S47J::FObj& A,const S47J::FObj& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 // Only the known serialization name differs: chapter-owned paperRatio vs routine ratio.
 // All FS47Doc strings, image bytes and complete sheets are compared; raw inputs remain byteoriginal.
 S47J::FObj SC07Doc(const S47J::FObj& Raw)
 {
  auto D=S47J::Clone(Raw);if(!D)return nullptr;
  if(D->HasField(TEXT("paperRatio"))&&!D->HasField(TEXT("ratio"))){D->SetField(TEXT("ratio"),D->GetField<EJson::None>(TEXT("paperRatio")));D->RemoveField(TEXT("paperRatio"));}return D;
 }
 bool SC07Bytes(const S47J::FObj& A,const S47J::FObj& B)
 {return A&&B&&S47J::Num(A,TEXT("bytes"),-1)==S47J::Num(B,TEXT("bytes"),-2)&&S47J::Str(A,TEXT("sha1"))==S47J::Str(B,TEXT("sha1"));}
 bool SC07Copy(const FString& From,const FString& To)
 {TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*From)&&FFileHelper::SaveArrayToFile(Bytes,*To)&&SC07Bytes(SC07File(From),SC07File(To));}

}
US47Chapter2* FS47SC07Contract::Act() const{return M&&M->Story?Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))):nullptr;}
void FS47SC07Contract::Next(int32 Value,double Delay){Stage=Value;Wait=Delay;Progress=FPlatformTime::Seconds();}
bool FS47SC07Contract::Check(bool Good,const FString& Why)
{
 ++Checks;auto R=S47J::New();S47J::Set(R,TEXT("stage"),Stage);S47J::Set(R,TEXT("pass"),Good);S47J::Set(R,TEXT("detail"),Why);Executed.Add(MakeShared<FJsonValueObject>(R));
 if(!Good){Finish(false,Why);return false;}UE_LOG(LogTemp,Display,TEXT("S47: SC07 PASS: %s"),*Why);return true;
}
void FS47SC07Contract::Finish(bool Good,const FString& Why)
{
 if(Done)return;if(Good&&!Check(InputUnchanged(),TEXT("parent checkpoint/result and both byteoriginal copies remain SHA/length-identical before PASS")))return;Done=true;if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("scenario"),TEXT("SC07"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("stage"),Stage);
 S47J::Set(O,TEXT("chain_id"),ChainId);S47J::Set(O,TEXT("ordinary_title_continues"),TitleContinues);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stores"),Stores);S47J::Set(O,TEXT("native_uses"),Uses);S47J::Set(O,TEXT("walk_samples"),Samples);S47J::Set(O,TEXT("walked_metres"),Metres);S47J::Set(O,TEXT("route"),TEXT("records-and-office"));S47J::Set(O,TEXT("normal_title_continue"),TitleContinued);
 S47J::Set(O,TEXT("prerequisite"),PriorReceipt);S47J::Set(O,TEXT("expected_restore_state"),Expected);S47J::Set(O,TEXT("checkpoint_file"),Root.IsEmpty()?nullptr:SC07File(Root/TEXT("sc07-complete.disk.json")));S47J::Set(O,TEXT("final_state"),M&&M->Flow?M->Flow->GameState():nullptr);S47J::Set(O,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(O,TEXT("source_method"),TEXT("Actual SC06 PASS and complete disk checkpoint; byteoriginal isolated case1 transfer followed by ordinary title Continue, with normal manual/Reader/title/Continue at every later cohort. Native Walker TestMove/Resolve, actual camera Pick/focus/HUD OnUse, real document modals, production archive board and telephone, physical Operations drawer/key/door/field-folder/note, disk/title Continue."));
 S47J::Set(O,TEXT("limits"),TEXT("SC01-SC06 are consumed from the named real predecessor, not replayed. The20:30 run-binder seed must already be actually retained; no source substitute or optional-document read quota. VHS and six private pages are not played or credited. API actions are not human input. No visual quality, audio listening, FPS, full matrix, installation or release acceptance."));
 O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("rays"),Rays);O->SetArrayField(TEXT("restores"),Restores);O->SetArrayField(TEXT("photographs"),Photos);
 S47J::Set(O,TEXT("checkpoint"),Root/TEXT("sc07-complete.disk.json"));const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 if(Good&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc07 PASS COMPLETE: %d checks, %d restores, %.3fm evidence=%s"),Checks,Stores,Metres,*Root);}
 else{UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc07 FAIL stage=%d: %s evidence=%s"),Stage,*Why,*Root);}FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:1);
}
void FS47SC07Contract::Path(TArray<FVector2D> Points,int32 NextStage){Route=MoveTemp(Points);Cursor=0;Last=M->Player->Pos;Next(NextStage,0);}
bool FS47SC07Contract::Walk()
{
 const FVector At=M->Player->Pos;const double Moved=FVector2D(At.X-Last.X,At.Z-Last.Z).Size();Last=At;
 if(Moved>.8){Finish(false,TEXT("native Walker discontinuity; no placement recovery"));return false;}Metres+=Moved;++Samples;
 if(!Route.IsValidIndex(Cursor)){M->Player->TestMove.Reset();M->Player->bTestRun=false;return true;}
 if(FPlatformTime::Seconds()-Progress>20){Finish(false,TEXT("native route blocked at ")+Route[Cursor].ToString());return false;}
 const FVector2D D=Route[Cursor]-FVector2D(At.X,At.Z);if(D.Size()<.065){++Cursor;Progress=FPlatformTime::Seconds();M->Player->TestMove.Reset();return Cursor==Route.Num();}
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Y);M->Player->Pitch=-.05;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,D.Size()/.35));M->Player->bTestRun=true;return false;
}
void FS47SC07Contract::FacePoint(const FVector& Three)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;Aim=S47Three::ToUE(Three);const FVector D=Three-S47Three::ToThree(M->Player->EyeUE());
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
bool FS47SC07Contract::Face(const FString& Id)
{
 const auto* I=M->Inter->Get(Id);FS47Obj Obj=I?I->Object:FS47Obj();if(!Obj)if(const auto* N=M->Web->Item(Id))Obj=M->Web->ObjAt(N->Node);
 const FBox Box=Obj.WorldBox();if(!Check(Obj&&Box.IsValid,TEXT("original interaction geometry: ")+Id))return false;FacePoint(Box.GetCenter());return true;
}
bool FS47SC07Contract::Use(const FString& Id)
{
 const auto Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Pick=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);
 auto R=S47J::New();S47J::Set(R,TEXT("id"),Id);S47J::Set(R,TEXT("picked"),Pick);S47J::Set(R,TEXT("focus"),Focus);S47J::Set(R,TEXT("eye"),Eye.ToString());S47J::Set(R,TEXT("forward"),Forward.ToString());Rays.Add(MakeShared<FJsonValueObject>(R));
 if(!Check(Pick==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary Use: ")+Id))return false;M->Player->OnUse(Pick);++Uses;return true;
}
bool FS47SC07Contract::Documents(bool Remember)
{
 TMap<FString,S47J::FObj> Now;for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object){auto D=V->AsObject();Now.Add(S47J::Str(D,TEXT("id")),SC07Doc(D));}
 for(const auto& P:Papers)if(!Check(Now.Contains(P.Key)&&SC07Same(Now[P.Key],P.Value),TEXT("all acquired document/image/stamp/sheets preserved: ")+P.Key))return false;if(Remember)Papers=MoveTemp(Now);return true;
}
bool FS47SC07Contract::InputUnchanged() const
{
 if(ChainId.IsEmpty()||!PriorReceipt)return false;
 for(const TCHAR* Kind:{TEXT("checkpoint"),TEXT("result")})
 {
  const auto Source=S47J::Obj(PriorReceipt,Kind),Copy=S47J::Obj(PriorReceipt,FCString::Strcmp(Kind,TEXT("checkpoint"))==0?TEXT("byteoriginal_checkpoint"):TEXT("byteoriginal_result"));
  if(!SC07Bytes(Source,SC07File(S47J::Str(Source,TEXT("path"))))||!SC07Bytes(Source,Copy)||!SC07Bytes(Copy,SC07File(S47J::Str(Copy,TEXT("path")))))return false;
 }
 return true;
}
bool FS47SC07Contract::ContinueDisk(const FString& Name,const S47J::FObj& State)
{
 M->Flow->ToTitle();FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Selected=Reader.ContinueSave(),Actual=M->Flow->Saves.ContinueSave();const auto Disk=Selected?Reader.Load(Selected->Id):nullptr;
 if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&Selected->CaseId==1&&Actual&&Selected->Id==Actual->Id&&SC07Same(Disk,State),TEXT("ordinary title and independent reader choose the actual full case1 cohort: ")+Name))return false;
 Expected=S47J::Clone(Disk);Pending=Name;
 if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens actual selected case1 disk: ")+Name))return false;
 ++Stores;++TitleContinues;TitleContinued=true;return true;
}
bool FS47SC07Contract::Store(const FString& Name,bool Final)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;
 const FString Block=M->Flow->SaveBlock();
 if(M->ModalOpen()||M->Story->IsCinematic()||M->World->bBusy||M->Flow->bStarting||!Block.IsEmpty()){if(FPlatformTime::Seconds()-Progress>25)Finish(false,TEXT("ordinary save gate blocked: ")+Name+TEXT(" / ")+Block);return false;}
 if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("ordinary gameplay case1 before manual save: ")+Name)||!Documents(true))return false;
 Expected=S47J::Clone(M->Flow->GameState());FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):nullptr;
 if(!Check(Meta&&Meta->CaseId==1&&Meta->Kind==TEXT("manual")&&SC07Same(Disk,Expected)&&FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("Flow.SaveBlock/manual write + independent full disk reader: ")+Name+TEXT(" / ")+Error))return false;
 if(Final&&!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("sc07-complete.disk.json"))),TEXT("stable actual SC07 complete checkpoint for next scenario")))return false;
 return ContinueDisk(Name,Disk);
}
bool FS47SC07Contract::Restored()
{
 const auto Actual=M->Flow->GameState(),WantCase=S47J::Obj(Expected,TEXT("case")),Pose=S47J::Obj(Expected,TEXT("pose"));
 if(!Check(M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&SC07Same(M->Flow->Carry,Expected),TEXT("normal case1/full Flow carries every original input field without developer restore: ")+Pending))return false;
 const auto WantSpine=S47J::Obj(Expected,TEXT("spine")),NowSpine=S47J::Obj(Actual,TEXT("spine"));const int32 WasVersion=S47J::Int(WantSpine,TEXT("v")),NowVersion=S47J::Int(NowSpine,TEXT("v"));
 if(!Check((WasVersion==1||WasVersion==2)&&NowVersion>=WasVersion&&NowVersion<=2,TEXT("legitimate Spine v1/v2 retained without downgrade: ")+Pending))return false;
 for(const TCHAR* Field:{TEXT("sc04d"),TEXT("sc05d")})if(S47J::Obj(WantSpine,Field)&&!Check(SC07Same(S47J::Obj(WantSpine,Field),S47J::Obj(NowSpine,Field)),FString(TEXT("all actual prior custody/pointing/facts retained: "))+Field+TEXT(" / ")+Pending))return false;
 if(!Check(M->World->Area==S47J::Str(Expected,TEXT("area"))&&M->Flow->CheckpointName()==S47J::Str(Expected,TEXT("checkpoint")),TEXT("actual saved area/checkpoint retained: ")+Pending))return false;
 if(!Check(S47J::Write(S47J::Obj(S47J::Obj(Actual,TEXT("case")),TEXT("ch2")))==S47J::Write(S47J::Obj(WantCase,TEXT("ch2")))&&S47J::Write(S47J::Obj(S47J::Obj(Actual,TEXT("case")),TEXT("s")))==S47J::Write(S47J::Obj(WantCase,TEXT("s")))&&M->Story->Clock()>=S47J::Num(WantCase,TEXT("clock"))&&M->Story->Clock()-S47J::Num(WantCase,TEXT("clock"))<4,TEXT("actual full Flow retains SC07 photo/control/source state and forward-only clock: ")+Pending))return false;
 if(!Check(S47J::Write(S47J::Obj(Actual,TEXT("office")))==S47J::Write(S47J::Obj(Expected,TEXT("office"))),TEXT("actual physical Office key/drawer/door/paper flags and acquisition stamp retained: ")+Pending))return false;
 if(!Check(S47J::Write(S47J::Obj(Actual,TEXT("doors")))==S47J::Write(S47J::Obj(Expected,TEXT("doors")))&&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.04&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.04&&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.001,TEXT("saved doors/real pose/camera restored: ")+Pending)||!Documents())return false;
 auto R=S47J::New();S47J::Set(R,TEXT("name"),Pending);S47J::Set(R,TEXT("actual"),Actual);Restores.Add(MakeShared<FJsonValueObject>(R));Pending.Reset();Last=M->Player->Pos;return true;
}
void FS47SC07Contract::Tick(double Dt)
{
 if(!Initialized){Initialized=true;FString Mode;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);Enabled=Mode==TEXT("sc07");Began=FPlatformTime::Seconds();}
 if(!Enabled||Done||!M||!M->Story||!M->Flow||!M->World||!M->Player||!M->Inter||!M->Hud||!M->UI||!M->Office||!Act())return;
 if(FPlatformTime::Seconds()-Began>300){Finish(false,TEXT("300-second wall timeout"));return;}if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy)return;if(Wait>0){Wait-=Dt;return;}
 if(Stage==0)
 {
  if(!Check(FParse::Param(FCommandLine::Get(),TEXT("S47Title")),TEXT("actual chain requires title startup, no chapter Jump fixture")))return;
  if(M->Mode!=US47Main::EMode::Title)return;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);User=FPaths::ConvertRelativePathToFull(User);FPaths::CollapseRelativeDirectories(User);
  if(!Check((User.StartsWith(TEXT("/tmp/"))||User.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),User)&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),User)&&FParse::Param(FCommandLine::Get(),TEXT("S47Title"))&&!M->Flow->bDevRun&&M->Flow->Saves.List().IsEmpty()&&!IFileManager::Get().DirectoryExists(*(FPaths::ProjectSavedDir()/TEXT("SC07Contract"))),TEXT("fresh isolated title profile before any case/proof mutation; no developer night")))return;
  for(const TCHAR* Flag:{TEXT("S47Jump"),TEXT("S47Face"),TEXT("S47Walk"),TEXT("S47WalkFrom"),TEXT("S47Place"),TEXT("S47Drive"),TEXT("S47Auto"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47ProductionViews"),TEXT("S47NightSequenceContract"),TEXT("S47HorrorContract"),TEXT("S47Ch1Drive"),TEXT("S47Ch2Drive")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Finish(false,FString(TEXT("independent Jump/pose/input/story driver is forbidden in actual chain: "))+Flag);return;}}
  Root=FPaths::ProjectSavedDir()/TEXT("SC07Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);
  FParse::Value(FCommandLine::Get(),TEXT("S47SC06Checkpoint="),PriorPath);FString Data,ReceiptText;const FString Receipt=FPaths::GetPath(PriorPath)/TEXT("result.json");const bool Loaded=FFileHelper::LoadFileToString(Data,*PriorPath)&&FFileHelper::LoadFileToString(ReceiptText,*Receipt);auto Prior=Loaded?S47J::Read(Data):nullptr;auto Result=Loaded?S47J::Read(ReceiptText):nullptr;
  if(!Check(Prior&&Result&&S47J::Str(Result,TEXT("status"))==TEXT("PASS")&&S47J::Str(Result,TEXT("scenario"))==TEXT("SC06")&&S47J::Bool(S47J::Obj(S47J::Obj(Prior,TEXT("case")),TEXT("s")),TEXT("filed")),TEXT("actual named SC06 PASS and complete two-exposure disk prerequisite; no substitute documents")))return;
  const auto Certified=S47J::Obj(Result,TEXT("checkpoint_file")),ActualFile=SC07File(PriorPath);
  if(!Check(Certified&&ActualFile&&S47J::Str(Certified,TEXT("path"))==PriorPath&&S47J::Num(Certified,TEXT("bytes"))==S47J::Num(ActualFile,TEXT("bytes"))&&S47J::Str(Certified,TEXT("sha1"))==S47J::Str(ActualFile,TEXT("sha1")),TEXT("SC06 PASS pins these exact prerequisite disk bytes")))return;
  const auto SC06=S47J::Obj(S47J::Obj(Prior,TEXT("case")),TEXT("s"));
  if(!Check(S47J::Int(SC06,TEXT("sc06Version"))==1&&S47J::Bool(SC06,TEXT("filed"))&&S47J::Bool(SC06,TEXT("concluded"))&&S47J::Bool(SC06,TEXT("observed"))&&S47J::Int(SC06,TEXT("dev1"))>=3&&S47J::Int(SC06,TEXT("dev2"))>=3&&S47J::Arr(Result,TEXT("photographs")).Num()==2,TEXT("actual SC06v1 complete controlled two-exposure proof is mandatory")))return;
  for(const TCHAR* Field:{TEXT("f1"),TEXT("f2")})
  {
   const auto Photo=FS47Photo::FromJson(S47J::Obj(SC06,Field));TArray<FColor> Pixels;int32 Width=0,Height=0;
   if(!Check(Photo&&Photo->Targets.Num()==2&&S47Image::DecodePixels(Photo->Url,Pixels,Width,Height)&&Width==US47FieldCamera::PRINT_W&&Height==US47FieldCamera::PRINT_H,FString(TEXT("real retained photographic bytes and measured coordinates decode: "))+Field))return;
  }
  PriorReceipt=S47J::New();S47J::Set(PriorReceipt,TEXT("checkpoint"),SC07File(PriorPath));S47J::Set(PriorReceipt,TEXT("result"),SC07File(Receipt));
  ChainId=S47J::Str(Result,TEXT("chain_id"));if(!Check(!ChainId.IsEmpty(),TEXT("exact non-empty actual predecessor chain_id; no local chain root")))return;
  const int32 InputSpine=S47J::Int(S47J::Obj(Prior,TEXT("spine")),TEXT("v"));if(!Check((InputSpine==1||InputSpine==2)&&S47J::Int(Prior,TEXT("timelineVersion"))==2,TEXT("actual new-night source with qualified Spine v1 or v2")))return;
  const FString CheckCopy=Root/TEXT("input-sc06.byteoriginal.disk.json"),ResultCopy=Root/TEXT("input-sc06.byteoriginal.result.json");
  if(!Check(SC07Copy(PriorPath,CheckCopy)&&SC07Copy(Receipt,ResultCopy),TEXT("retain byteoriginal parent checkpoint/result without reserialization")))return;
  S47J::Set(PriorReceipt,TEXT("byteoriginal_checkpoint"),SC07File(CheckCopy));S47J::Set(PriorReceipt,TEXT("byteoriginal_result"),SC07File(ResultCopy));
  FString CopiedState,CopiedResult;if(!Check(FFileHelper::LoadFileToString(CopiedState,*CheckCopy)&&FFileHelper::LoadFileToString(CopiedResult,*ResultCopy)&&SC07Same(S47J::Read(CopiedState),Prior)&&SC07Same(S47J::Read(CopiedResult),Result),TEXT("parsed predecessor state/PASS equal the exact byteoriginal evidence copies")))return;
  for(const TCHAR* Part:{TEXT("s"),TEXT("ch2")})for(const auto& V:S47J::Arr(S47J::Obj(S47J::Obj(Prior,TEXT("case")),Part),TEXT("sources")))if(V&&V->Type==EJson::Object){const auto D=V->AsObject();Papers.Add(S47J::Str(D,TEXT("id")),SC07Doc(D));}
  if(!Check(!Papers.IsEmpty()&&InputUnchanged(),TEXT("actual full acquired source inventory and immutable raw input pins")))return;
  Expected=S47J::Clone(Prior);FS47SaveCard Card;Card.Chapter=TEXT("SC06 actual completed checkpoint");Card.Place=S47J::Str(Prior,TEXT("area"));Card.Clock=S47Text::ClockText(S47J::Num(S47J::Obj(Prior,TEXT("case")),TEXT("clock")),true);FString Error;
  const auto Seed=M->Flow->Saves.Save(1,TEXT("manual"),0,Expected,Card,Error);FS47SaveStore SeedReader(M->Flow->Saves.Dir);SeedReader.Init();const auto Imported=Seed?SeedReader.Load(Seed->Id):nullptr;
  if(!Check(Seed&&Seed->CaseId==1&&SC07Same(Imported,Prior),TEXT("only exact parent full state transferred into isolated case1/manual; independent reader: ")+Error))return;
  if(!ContinueDisk(TEXT("actual-sc06-input"),Imported))return;Next(1,.3);return;
 }
 if(!Pending.IsEmpty()&&!Restored())return;if(M->Mode!=US47Main::EMode::Play)return;auto* C=Act();
 switch(Stage)
 {
 case 1:
  if(!Check(C->S.SequenceVersion==1&&C->S.Stage==TEXT("ward-call")&&!C->S.bAnswered&&M->Story->ChapterKey==TEXT("ch2")&&M->Story->Clock()>=3*3600+40*60,TEXT("actual SC06 begins fresh opt-in SC07 at 03:40")))return;
  if(!Check(M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("obs_runbinder")&&D.Page.Contains(TEXT("AMENDED / SERVICE COPY"));}),TEXT("actual 20:30 run-binder source is already retained, never fabricated here")))return;
  if(!Store(TEXT("before-archive")))return;Next(2);return;
 case 2:if(!Face(TEXT("phone")))return;Next(3);return;
 case 3:if(!Use(TEXT("phone")))return;Next(4,.3);return;
 case 4:if(M->Story->IsCinematic())return;if(!Check(C->S.bAnswered,TEXT("actual supervisor telephone completes")))return;Path({{.6,1.4},{-2.4,1.4},{-2.4,3.7}},5);return;
 case 5:if(!Walk())return;if(!Face(TEXT("doorSouth")))return;Next(6);return;
 case 6:if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(TEXT("doorSouth")))return;Next(7,.7);return;
 case 7:Path({{-2.4,5.7},{-3.9,5.7}},8);return;
 case 8:if(!Walk())return;if(!Face(TEXT("recordsDoor")))return;Next(9);return;
 case 9:if(!M->Web->IsDoorOpen(TEXT("recordsDoor"))&&!Use(TEXT("recordsDoor")))return;Next(10,.7);return;
 case 10:Path({{-3.9,7.5},{-4.95,7.65}},11);return;
 case 11:if(!Walk())return;if(!Face(TEXT("fieldBox")))return;Next(12);return;
 case 12:if(!Use(TEXT("fieldBox")))return;Next(13);return;
 case 13:if(!Check(M->ModalOpen()&&C->S.Read.bOriginal,TEXT("actual E07 original modal and plate source opened")))return;M->UI->Close(true);if(!Check(C->S.Read.bIndex,TEXT("matching E08 sleeve is retained by the actual field-box close callback")))return;Path({{-3.9,7.5},{-2.3,7.5},{-2.3,9.6}},14);return;
 case 14:if(!Walk())return;if(!Face(TEXT("binder")))return;Next(15);return;
 case 15:if(!Use(TEXT("binder")))return;Next(16);return;
 case 16:if(!Check(M->ModalOpen()&&C->S.Read.bAmended,TEXT("actual E06 amended service copy opened")))return;M->UI->Close(true);if(!Check(C->S.bBinderCompared,TEXT("actual E06 close compares the retained 20:30 stamp without inventing the crossed-out content")))return;if(!Store(TEXT("original-and-amended-with-seed")))return;Next(17);return;
 case 17:Path({{-2.3,7.5}},18);return;
 case 18:if(!Walk())return;if(!Face(TEXT("lineage")))return;Next(19);return;
 case 19:if(!Use(TEXT("lineage")))return;Next(20);return;
 case 20:if(!Check(M->ModalOpen()&&C->S.Read.bLineage,TEXT("actual B-12 lineage card opened")))return;M->UI->Close(true);Path({{-3.9,7.5}},21);return;
 case 21:if(!Walk())return;if(!Face(TEXT("workTable")))return;Next(22);return;
 case 22:if(!Use(TEXT("workTable")))return;Next(23);return;
 case 23:
 {
  auto* B=C->Book.Get();if(!Check(B&&M->ModalOpen(),TEXT("actual records table offers production P04/P05 board")))return;
  const auto A=B->Connect(TEXT("e07.abc"),TEXT("e06.omit"),false),Mark=B->Connect(TEXT("e07.mark"),TEXT("e06.fault"),false);const auto P04=B->Record(TEXT("P04"));
  const auto Id=B->Connect(TEXT("b12.id"),TEXT("e08.id"),false),Symbol=B->Connect(TEXT("b12.mark"),TEXT("e08.mark"),false),Place=B->Connect(TEXT("b12.site"),TEXT("e08.dest"),false);const auto P05=B->Record(TEXT("P05"));
  if(!Check(A&&A->bOk&&Mark&&Mark->bOk&&P04&&P04->bOk&&Id&&Id->bOk&&Symbol&&Symbol->bOk&&Place&&Place->bOk&&P05&&P05->bOk&&C->S.bP04&&C->S.bP05,TEXT("actual authored ABC/C, retained mark, STATION01 and triangle-with-bar rules record P04/P05")))return;
  M->UI->Close(true);if(!Store(TEXT("archive-findings")))return;Next(24);return;
 }
 case 24:Path({{-2.3,7.5},{-2.3,9.25}},25);return;
 case 25:if(!Walk())return;if(!Face(TEXT("recPhone")))return;Next(26);return;
 case 26:if(!Use(TEXT("recPhone")))return;Next(27,.3);return;
 case 27:if(M->Story->IsCinematic())return;if(!Check(C->S.bCalled&&C->S.Stage==TEXT("complete")&&!M->Story->Begun.Contains(TEXT("ch3")),TEXT("Nora's actual call completes and physical Operations still guards departure")))return;if(!Store(TEXT("after-nora-before-office")))return;Next(28);return;
 case 28:Path({{-2.3,7.5},{-3.9,7.5},{-3.9,6.05},{-1.5,6.05},{-1.5,5.6},{.45,5.6},{.45,6.05}},29);return;
 case 29:if(!Walk())return;if(!Face(TEXT("office:drawer")))return;Next(30);return;
 case 30:if(!M->Office->HasKey()){if(!Use(TEXT("office:drawer")))return;Next(31,.7);}else Next(33,0);return;
 case 31:if(!Face(TEXT("office:key")))return;Next(32);return;
 case 32:if(!Use(TEXT("office:key"))||!Check(M->Office->HasKey(),TEXT("actual tagged Operations spare retained")))return;Next(33,0);return;
 case 33:if(!Face(TEXT("office:door")))return;Next(34);return;
 case 34:if(!M->Office->DoorOpen()&&!Use(TEXT("office:door")))return;Next(35,.8);return;
 case 35:Path({{.45,6.6},{.45,7.4},{.30,8.1},{.08,8.98}},36);return;
 case 36:if(!Walk())return;if(!Check(M->Office->Contains(M->Player->Pos)&&M->Office->WasUnlocked(),TEXT("native Walker enters actual Operations portal"))||!Face(TEXT("office:folder")))return;Next(37);return;
 case 37:if(!Use(TEXT("office:folder")))return;Next(38);return;
 case 38:
 {
  const auto* Index=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_INDEX");});
  if(!Check(M->ModalOpen()&&M->Office->HasFieldSheet()&&Index&&!Index->Page.IsEmpty(),TEXT("actual field-folder paper retains EV_DALE_INDEX with measured acquisition time")))return;M->UI->Close(true);
  if(!M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_NOTE");})&&!Check(!C->ReadyForDeparture()&&!M->Office->WantsDeparture(),TEXT("index alone does not falsely complete required folded-note route")))return;
  if(!Store(TEXT("actual-field-folder")))return;Next(39);return;
 }
 case 39:Path({{.28,8.90}},40);return;
 case 40:if(!Walk())return;if(!Face(TEXT("office:note")))return;Next(41);return;
 case 41:if(!Use(TEXT("office:note")))return;Next(42);return;
 case 42:
 {
  const auto* Note=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_NOTE");});
  if(!Check(M->ModalOpen()&&Note&&!Note->Page.IsEmpty(),TEXT("actual folded note EV_DALE_NOTE opened and retained")))return;M->UI->Close(true);
  if(!Check(C->ReadyForDeparture(),TEXT("the two actual field papers suffice; optional collection and unavailable VHS are not gates")))return;
  if(!Store(TEXT("actual-note-and-index")))return;Next(43);return;
 }
 case 43:Path({{.45,8.1},{.45,7.5},{.65,7.45}},44);return;
 case 44:if(!Walk())return;if(!Face(TEXT("office:depart")))return;Next(45);return;
 case 45:if(!Use(TEXT("office:depart")))return;Next(46,.8);return;
 case 46:if(M->Story->IsCinematic())return;if(!Check(M->Office->DepartureReleased()&&C->ReadyForDeparture()&&M->Story->Clock()>=3*3600+55*60&&!M->Story->Begun.Contains(TEXT("ch3")),TEXT("actual departure card completes SC07 at 03:55 and leaves SC08 to its own scenario")))return;if(!Store(TEXT("sc07-complete"),true))return;Next(47);return;
 case 47:if(!Documents(true))return;Next(48);return;
 case 48:if(!Store(TEXT("ordinary-title-continue")))return;Next(49,.3);return;
 case 49:if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&TitleContinued&&TitleContinues==Stores&&Restores.Num()==Stores&&Uses>=9&&Stores>=6&&Metres>20&&Samples>100&&C->ReadyForDeparture()&&M->Office->DepartureReleased(),TEXT("actual archive/Office route and all source-preserving disk/title cohorts executed")))return;Finish(true,TEXT("actual corrected records, retained binder seed and Dale field folder; optional sources remain optional"));return;
 default:Finish(false,TEXT("unknown SC07 stage"));return;
 }
}
void FS47SC07Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC07Contract> Run;if(Owner.Get()!=Main){Owner=Main;Run=MakeShareable(new FS47SC07Contract(Main));}const auto KeepAlive=Run;if(KeepAlive)KeepAlive->Tick(Dt);
}
