#include "S47SC06Contract.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Yard.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 FString SC06Text(const TSharedRef<SWidget>& W)
 {
  FString Out;if(W->GetTypeAsString()==TEXT("STextBlock"))Out=StaticCastSharedRef<STextBlock>(W)->GetText().ToString();
  auto* Children=W->GetChildren();for(int32 I=0;Children&&I<Children->Num();++I)Out+=TEXT("\n")+SC06Text(Children->GetChildAt(I));return Out;
 }
 void SC06Widgets(const TSharedRef<SWidget>& W,const FString& Type,TArray<TSharedRef<SWidget>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==Type)Out.Add(W);
  auto* Children=W->GetChildren();for(int32 I=0;Children&&I<Children->Num();++I)SC06Widgets(Children->GetChildAt(I),Type,Out);
 }
 bool SC06Click(const TSharedRef<SWidget>& W,const FVector2D& Local)
 {
  const auto& Geometry=W->GetCachedGeometry();if(Geometry.GetLocalSize().X<=0)return false;
  const FVector2D At=Geometry.LocalToAbsolute(Local);TSet<FKey> Pressed;Pressed.Add(EKeys::LeftMouseButton);
  const FPointerEvent Down(0,At,At,Pressed,EKeys::LeftMouseButton,0,FModifierKeysState());
  const FPointerEvent Up(0,At,At,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());
  return W->OnMouseButtonDown(Geometry,Down).IsEventHandled()&&W->OnMouseButtonUp(Geometry,Up).IsEventHandled();
 }
 S47J::FObj SC06File(const FString& Path)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*Path))return nullptr;FSHAHash H;FSHA1::HashBuffer(B.GetData(),B.Num(),H.Hash);
  auto O=S47J::New();S47J::Set(O,TEXT("path"),Path);S47J::Set(O,TEXT("bytes"),B.Num());S47J::Set(O,TEXT("sha1"),H.ToString());return O;
 }
 bool SC06Same(const S47J::FObj& A,const S47J::FObj& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 // Only the known serialization name differs: chapter-owned paperRatio vs routine ratio.
 // All FS47Doc strings, image bytes and complete sheets are compared; raw inputs remain byteoriginal.
 S47J::FObj SC06Doc(const S47J::FObj& Raw)
 {
  auto D=S47J::Clone(Raw);if(!D)return nullptr;
  if(D->HasField(TEXT("paperRatio"))&&!D->HasField(TEXT("ratio"))){D->SetField(TEXT("ratio"),D->GetField<EJson::None>(TEXT("paperRatio")));D->RemoveField(TEXT("paperRatio"));}return D;
 }
 bool SC06Bytes(const S47J::FObj& A,const S47J::FObj& B)
 {return A&&B&&S47J::Num(A,TEXT("bytes"),-1)==S47J::Num(B,TEXT("bytes"),-2)&&S47J::Str(A,TEXT("sha1"))==S47J::Str(B,TEXT("sha1"));}
 bool SC06Copy(const FString& From,const FString& To)
 {TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*From)&&FFileHelper::SaveArrayToFile(Bytes,*To)&&SC06Bytes(SC06File(From),SC06File(To));}

}
US47Chapter1* FS47SC06Contract::Act() const{return M&&M->Story?Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1"))):nullptr;}
void FS47SC06Contract::Next(int32 Value,double Delay){Stage=Value;Wait=Delay;Progress=FPlatformTime::Seconds();}
bool FS47SC06Contract::Check(bool Good,const FString& Why)
{
 ++Checks;auto R=S47J::New();S47J::Set(R,TEXT("stage"),Stage);S47J::Set(R,TEXT("pass"),Good);S47J::Set(R,TEXT("detail"),Why);Executed.Add(MakeShared<FJsonValueObject>(R));
 if(!Good){Finish(false,Why);return false;}UE_LOG(LogTemp,Display,TEXT("S47: SC06 PASS: %s"),*Why);return true;
}
void FS47SC06Contract::Finish(bool Good,const FString& Why)
{
 if(Done)return;if(Good&&!Check(InputUnchanged(),TEXT("parent checkpoint/result and both byteoriginal copies remain SHA/length-identical before PASS")))return;Done=true;if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("scenario"),TEXT("SC06"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("stage"),Stage);
 S47J::Set(O,TEXT("chain_id"),ChainId);S47J::Set(O,TEXT("ordinary_title_continues"),TitleContinues);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stores"),Stores);S47J::Set(O,TEXT("native_uses"),Uses);S47J::Set(O,TEXT("walk_samples"),Samples);S47J::Set(O,TEXT("walked_metres"),Metres);S47J::Set(O,TEXT("method"),Method);S47J::Set(O,TEXT("normal_title_continue"),TitleContinued);
 S47J::Set(O,TEXT("prerequisite"),PriorReceipt);S47J::Set(O,TEXT("expected_restore_state"),Expected);S47J::Set(O,TEXT("checkpoint_file"),Root.IsEmpty()?nullptr:SC06File(Root/TEXT("sc06-complete.disk.json")));S47J::Set(O,TEXT("final_state"),M&&M->Flow?M->Flow->GameState():nullptr);S47J::Set(O,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(O,TEXT("source_method"),TEXT("Actual SC05 PASS and complete disk checkpoint; byteoriginal isolated case1 transfer followed by ordinary title Continue, with normal manual/Reader/title/Continue at every later cohort. Native Walker TestMove/Resolve, actual camera Pick/focus/HUD OnUse, actual Slate button/print pointer callbacks, real field-camera RHI exposures, production board rules, real fog/reprint, disk/title Continue."));
 S47J::Set(O,TEXT("limits"),TEXT("SC01-SC05 are consumed from the named real predecessor, not replayed. One selected B-12 method per process; run passive and active separately. API actions are not human input. Image payload/geometry checks do not certify visual quality, audio listening, FPS, full matrix, installation or release."));
 O->SetArrayField(TEXT("executed_checks"),Executed);O->SetArrayField(TEXT("rays"),Rays);O->SetArrayField(TEXT("restores"),Restores);O->SetArrayField(TEXT("photographs"),Photos);
 S47J::Set(O,TEXT("checkpoint"),Root/TEXT("sc06-complete.disk.json"));const bool Written=!Root.IsEmpty()&&FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 if(Good&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc06 PASS COMPLETE: %d checks, %d restores, %.3fm evidence=%s"),Checks,Stores,Metres,*Root);}
 else{UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc06 FAIL stage=%d: %s evidence=%s"),Stage,*Why,*Root);}FPlatformMisc::RequestExitWithStatus(false,Good&&Written?0:1);
}
void FS47SC06Contract::Path(TArray<FVector2D> Points,int32 NextStage){Route=MoveTemp(Points);Cursor=0;Last=M->Player->Pos;Next(NextStage,0);}
bool FS47SC06Contract::Walk()
{
 const FVector At=M->Player->Pos;const double Moved=FVector2D(At.X-Last.X,At.Z-Last.Z).Size();Last=At;
 if(Moved>.8){Finish(false,TEXT("native Walker discontinuity; no placement recovery"));return false;}Metres+=Moved;++Samples;
 if(!Route.IsValidIndex(Cursor)){M->Player->TestMove.Reset();M->Player->bTestRun=false;return true;}
 if(FPlatformTime::Seconds()-Progress>20){Finish(false,TEXT("native route blocked at ")+Route[Cursor].ToString());return false;}
 const FVector2D D=Route[Cursor]-FVector2D(At.X,At.Z);if(D.Size()<.065){++Cursor;Progress=FPlatformTime::Seconds();M->Player->TestMove.Reset();return Cursor==Route.Num();}
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Y);M->Player->Pitch=-.05;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,D.Size()/.35));M->Player->bTestRun=true;return false;
}
void FS47SC06Contract::FacePoint(const FVector& Three)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;Aim=S47Three::ToUE(Three);const FVector D=Three-S47Three::ToThree(M->Player->EyeUE());
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
bool FS47SC06Contract::Face(const FString& Id)
{
 const auto* I=M->Inter->Get(Id);FS47Obj Obj=I?I->Object:FS47Obj();if(!Obj)if(const auto* N=M->Web->Item(Id))Obj=M->Web->ObjAt(N->Node);
 const FBox Box=Obj.WorldBox();if(!Check(Obj&&Box.IsValid,TEXT("original interaction geometry: ")+Id))return false;FacePoint(Box.GetCenter());return true;
}
bool FS47SC06Contract::Use(const FString& Id)
{
 const auto Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();const FString Pick=M->Inter->Pick(Eye,Forward),Focus=M->Inter->Update(Eye,Forward);
 auto R=S47J::New();S47J::Set(R,TEXT("id"),Id);S47J::Set(R,TEXT("picked"),Pick);S47J::Set(R,TEXT("focus"),Focus);S47J::Set(R,TEXT("eye"),Eye.ToString());S47J::Set(R,TEXT("forward"),Forward.ToString());Rays.Add(MakeShared<FJsonValueObject>(R));
 if(!Check(Pick==Id&&Focus==Id&&M->Player->OnUse&&FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal())>.995,TEXT("actual camera ray/focus/ordinary Use: ")+Id))return false;M->Player->OnUse(Pick);++Uses;return true;
}
bool FS47SC06Contract::Button(const FString& Text)
{
 const auto Modal=M->Hud->ModalWidget();TArray<TSharedRef<SWidget>> Buttons;if(Modal)SC06Widgets(Modal.ToSharedRef(),TEXT("SButton"),Buttons);
 for(const auto& B:Buttons)if(B->IsEnabled()&&SC06Text(B).Contains(Text)){StaticCastSharedRef<SButton>(B)->SimulateClick();return Check(true,TEXT("offered Slate SButton::SimulateClick API: ")+Text);}
 return Check(false,TEXT("offered Slate button missing: ")+Text);
}
bool FS47SC06Contract::MarkPrint(int32 Which)
{
 const auto& P=Which==1?Act()->S.F1:Act()->S.F2;if(!Check(P&&P->Targets.Num()>0,TEXT("real exposure has measured print targets")))return false;
 const auto Modal=M->Hud->ModalWidget();TArray<TSharedRef<SWidget>> Prints;if(Modal)SC06Widgets(Modal.ToSharedRef(),TEXT("SS47Print"),Prints);
 const int32 Index=Which==1?0:1;if(!Check(Prints.IsValidIndex(Index),TEXT("ordinary loupe/two-print widget exists")))return false;
 const auto UV=P->Targets[0].Value;const auto& W=Prints[Index];const double Width=W->GetCachedGeometry().GetLocalSize().X;
 const FVector2D Local((US47FieldCamera::PRINT_X+UV.X*US47FieldCamera::PHOTO_W)/US47FieldCamera::PRINT_W*Width,(US47FieldCamera::PRINT_Y+UV.Y*US47FieldCamera::PHOTO_H)/US47FieldCamera::PRINT_W*Width);
 return Check(SC06Click(W,Local)&&(Which==1?Act()->S.Mark1.IsSet():Act()->S.Mark2.IsSet()),TEXT("actual pointer callback marks measured feature without editing original image"));
}
bool FS47SC06Contract::Photograph()
{
 auto* C=Act();if(!Check(C->Check().bOk,TEXT("actual field-camera readiness: ")+C->Check().Text))return false;C->Shutter();const auto& P=Frame==1?C->S.F1:C->S.F2;
 TArray<FColor> Pixels;int32 Width=0,Height=0;if(!Check(P&&S47Image::DecodePixels(P->Url,Pixels,Width,Height)&&Width==US47FieldCamera::PRINT_W&&Height==US47FieldCamera::PRINT_H&&P->Targets.Num()==2,TEXT("real RHI exposure produces complete image and both target coordinates")))return false;
 TArray<uint8> Bytes;const FString Path=Root/FString::Printf(TEXT("frame%02d.jpg"),Frame);
 if(!Check(FBase64::Decode(P->Url.Mid(P->Url.Find(TEXT(","))+1),Bytes)&&FFileHelper::SaveArrayToFile(Bytes,*Path),TEXT("retain actual photographic bytes")))return false;auto Row=SC06File(Path);S47J::Set(Row,TEXT("photo"),P->ToJson());Photos.Add(MakeShared<FJsonValueObject>(Row));return true;
}
bool FS47SC06Contract::Documents(bool Remember)
{
 TMap<FString,S47J::FObj> Now;for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object){auto D=V->AsObject();Now.Add(S47J::Str(D,TEXT("id")),SC06Doc(D));}
 for(const auto& P:Papers)if(!Check(Now.Contains(P.Key)&&SC06Same(Now[P.Key],P.Value),TEXT("all acquired document/image/stamp/sheets preserved: ")+P.Key))return false;if(Remember)Papers=MoveTemp(Now);return true;
}
bool FS47SC06Contract::InputUnchanged() const
{
 if(ChainId.IsEmpty()||!PriorReceipt)return false;
 for(const TCHAR* Kind:{TEXT("checkpoint"),TEXT("result")})
 {
  const auto Source=S47J::Obj(PriorReceipt,Kind),Copy=S47J::Obj(PriorReceipt,FCString::Strcmp(Kind,TEXT("checkpoint"))==0?TEXT("byteoriginal_checkpoint"):TEXT("byteoriginal_result"));
  if(!SC06Bytes(Source,SC06File(S47J::Str(Source,TEXT("path"))))||!SC06Bytes(Source,Copy)||!SC06Bytes(Copy,SC06File(S47J::Str(Copy,TEXT("path")))))return false;
 }
 return true;
}
bool FS47SC06Contract::ContinueDisk(const FString& Name,const S47J::FObj& State)
{
 M->Flow->ToTitle();FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Selected=Reader.ContinueSave(),Actual=M->Flow->Saves.ContinueSave();const auto Disk=Selected?Reader.Load(Selected->Id):nullptr;
 if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&Selected->CaseId==1&&Actual&&Selected->Id==Actual->Id&&SC06Same(Disk,State),TEXT("ordinary title and independent reader choose the actual full case1 cohort: ")+Name))return false;
 Expected=S47J::Clone(Disk);Pending=Name;
 if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens actual selected case1 disk: ")+Name))return false;
 ++Stores;++TitleContinues;TitleContinued=true;return true;
}
bool FS47SC06Contract::Store(const FString& Name,bool Final)
{
 M->Player->TestMove.Reset();M->Player->bTestRun=false;
 const FString Block=M->Flow->SaveBlock();
 if(M->ModalOpen()||M->Story->IsCinematic()||M->World->bBusy||M->Flow->bStarting||!Block.IsEmpty()){if(FPlatformTime::Seconds()-Progress>25)Finish(false,TEXT("ordinary save gate blocked: ")+Name+TEXT(" / ")+Block);return false;}
 if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("ordinary gameplay case1 before manual save: ")+Name)||!Documents(true))return false;
 Expected=S47J::Clone(M->Flow->GameState());FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):nullptr;
 if(!Check(Meta&&Meta->CaseId==1&&Meta->Kind==TEXT("manual")&&SC06Same(Disk,Expected)&&FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("Flow.SaveBlock/manual write + independent full disk reader: ")+Name+TEXT(" / ")+Error))return false;
 if(Final&&!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("sc06-complete.disk.json"))),TEXT("stable actual SC06 complete checkpoint for next scenario")))return false;
 return ContinueDisk(Name,Disk);
}
bool FS47SC06Contract::Restored()
{
 const auto Actual=M->Flow->GameState(),WantCase=S47J::Obj(Expected,TEXT("case")),Pose=S47J::Obj(Expected,TEXT("pose"));
 if(!Check(M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&SC06Same(M->Flow->Carry,Expected),TEXT("normal case1/full Flow carries every original input field without developer restore: ")+Pending))return false;
 const auto WantSpine=S47J::Obj(Expected,TEXT("spine")),NowSpine=S47J::Obj(Actual,TEXT("spine"));const int32 WasVersion=S47J::Int(WantSpine,TEXT("v")),NowVersion=S47J::Int(NowSpine,TEXT("v"));
 if(!Check((WasVersion==1||WasVersion==2)&&NowVersion>=WasVersion&&NowVersion<=2,TEXT("legitimate Spine v1/v2 retained without downgrade: ")+Pending))return false;
 for(const TCHAR* Field:{TEXT("sc04d"),TEXT("sc05d")})if(S47J::Obj(WantSpine,Field)&&!Check(SC06Same(S47J::Obj(WantSpine,Field),S47J::Obj(NowSpine,Field)),FString(TEXT("all actual prior custody/pointing/facts retained: "))+Field+TEXT(" / ")+Pending))return false;
 if(!Check(M->World->Area==S47J::Str(Expected,TEXT("area"))&&M->Flow->CheckpointName()==S47J::Str(Expected,TEXT("checkpoint")),TEXT("actual saved area/checkpoint retained: ")+Pending))return false;
 if(!Check(S47J::Write(S47J::Obj(S47J::Obj(Actual,TEXT("case")),TEXT("s")))==S47J::Write(S47J::Obj(WantCase,TEXT("s")))&&M->Story->Clock()>=S47J::Num(WantCase,TEXT("clock"))&&M->Story->Clock()-S47J::Num(WantCase,TEXT("clock"))<4,TEXT("actual full Flow retains SC06 photo/control/source state and forward-only clock: ")+Pending))return false;
 if(!Check(S47J::Write(S47J::Obj(Actual,TEXT("doors")))==S47J::Write(S47J::Obj(Expected,TEXT("doors")))&&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.04&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.04&&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.001,TEXT("saved doors/real pose/camera restored: ")+Pending)||!Documents())return false;
 auto R=S47J::New();S47J::Set(R,TEXT("name"),Pending);S47J::Set(R,TEXT("actual"),Actual);Restores.Add(MakeShared<FJsonValueObject>(R));Pending.Reset();Last=M->Player->Pos;return true;
}
void FS47SC06Contract::Tick(double Dt)
{
 if(!Initialized){Initialized=true;FString Mode;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);Enabled=Mode==TEXT("sc06");Began=FPlatformTime::Seconds();}
 if(!Enabled||Done||!M||!M->Story||!M->Flow||!M->World||!M->Player||!M->Inter||!M->Hud||!M->UI||!Act())return;
 if(FPlatformTime::Seconds()-Began>420){Finish(false,TEXT("420-second wall timeout"));return;}if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy)return;if(Wait>0){Wait-=Dt;return;}
 if(Stage==0)
 {
  if(!Check(FParse::Param(FCommandLine::Get(),TEXT("S47Title")),TEXT("actual chain requires title startup, no chapter Jump fixture")))return;
  if(M->Mode!=US47Main::EMode::Title)return;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);User=FPaths::ConvertRelativePathToFull(User);FPaths::CollapseRelativeDirectories(User);
  if(!Check((User.StartsWith(TEXT("/tmp/"))||User.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),User)&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),User)&&FParse::Param(FCommandLine::Get(),TEXT("S47Title"))&&!M->Flow->bDevRun&&M->Flow->Saves.List().IsEmpty()&&!IFileManager::Get().DirectoryExists(*(FPaths::ProjectSavedDir()/TEXT("SC06Contract"))),TEXT("fresh isolated title profile before any case/proof mutation; no developer night")))return;
  for(const TCHAR* Flag:{TEXT("S47Jump"),TEXT("S47Face"),TEXT("S47Walk"),TEXT("S47WalkFrom"),TEXT("S47Place"),TEXT("S47Drive"),TEXT("S47Auto"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47ProductionViews"),TEXT("S47NightSequenceContract"),TEXT("S47HorrorContract"),TEXT("S47Ch1Drive"),TEXT("S47Ch2Drive")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Finish(false,FString(TEXT("independent Jump/pose/input/story driver is forbidden in actual chain: "))+Flag);return;}}
  Root=FPaths::ProjectSavedDir()/TEXT("SC06Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);IFileManager::Get().MakeDirectory(*Root,true);
  FParse::Value(FCommandLine::Get(),TEXT("S47SC05Checkpoint="),PriorPath);FString Data,ReceiptText;const FString Receipt=FPaths::GetPath(PriorPath)/TEXT("result.json");const bool Loaded=FFileHelper::LoadFileToString(Data,*PriorPath)&&FFileHelper::LoadFileToString(ReceiptText,*Receipt);auto Prior=Loaded?S47J::Read(Data):nullptr;auto Result=Loaded?S47J::Read(ReceiptText):nullptr;
  if(!Check(Prior&&Result&&S47J::Str(Result,TEXT("status"))==TEXT("PASS")&&S47J::Str(Result,TEXT("scenario"))==TEXT("SC05")&&S47J::Obj(Prior,TEXT("case")),TEXT("actual named SC05 PASS and complete disk prerequisite; no substitute documents")))return;
  if(!Check(S47J::Str(Prior,TEXT("checkpoint"))==TEXT("chapter1"),TEXT("actual SC05 complete checkpoint is chapter1"))||!Check(S47J::Bool(S47J::Obj(S47J::Obj(Prior,TEXT("spine")),TEXT("sc05d")),TEXT("done"))&&S47J::Int(S47J::Obj(S47J::Obj(Prior,TEXT("case")),TEXT("s")),TEXT("sc06Version"))==1,TEXT("actual completed SC05 opens opt-in SC06; no chapter factory")))return;
  const auto Certified=S47J::Obj(Result,TEXT("checkpoint_file")),ActualFile=SC06File(PriorPath);
  if(!Check(Certified&&ActualFile&&S47J::Str(Certified,TEXT("path"))==PriorPath&&S47J::Num(Certified,TEXT("bytes"))==S47J::Num(ActualFile,TEXT("bytes"))&&S47J::Str(Certified,TEXT("sha1"))==S47J::Str(ActualFile,TEXT("sha1")),TEXT("SC05 PASS pins these exact prerequisite disk bytes")))return;
  PriorReceipt=S47J::New();S47J::Set(PriorReceipt,TEXT("checkpoint"),SC06File(PriorPath));S47J::Set(PriorReceipt,TEXT("result"),SC06File(Receipt));
  ChainId=S47J::Str(Result,TEXT("chain_id"));if(!Check(!ChainId.IsEmpty(),TEXT("exact non-empty actual predecessor chain_id; no local chain root")))return;
  const int32 InputSpine=S47J::Int(S47J::Obj(Prior,TEXT("spine")),TEXT("v"));if(!Check((InputSpine==1||InputSpine==2)&&S47J::Int(Prior,TEXT("timelineVersion"))==2,TEXT("actual new-night source with qualified Spine v1 or v2")))return;
  const FString CheckCopy=Root/TEXT("input-sc05.byteoriginal.disk.json"),ResultCopy=Root/TEXT("input-sc05.byteoriginal.result.json");
  if(!Check(SC06Copy(PriorPath,CheckCopy)&&SC06Copy(Receipt,ResultCopy),TEXT("retain byteoriginal parent checkpoint/result without reserialization")))return;
  S47J::Set(PriorReceipt,TEXT("byteoriginal_checkpoint"),SC06File(CheckCopy));S47J::Set(PriorReceipt,TEXT("byteoriginal_result"),SC06File(ResultCopy));
  FString CopiedState,CopiedResult;if(!Check(FFileHelper::LoadFileToString(CopiedState,*CheckCopy)&&FFileHelper::LoadFileToString(CopiedResult,*ResultCopy)&&SC06Same(S47J::Read(CopiedState),Prior)&&SC06Same(S47J::Read(CopiedResult),Result),TEXT("parsed predecessor state/PASS equal the exact byteoriginal evidence copies")))return;
  for(const TCHAR* Part:{TEXT("s"),TEXT("ch2")})for(const auto& V:S47J::Arr(S47J::Obj(S47J::Obj(Prior,TEXT("case")),Part),TEXT("sources")))if(V&&V->Type==EJson::Object){const auto D=V->AsObject();Papers.Add(S47J::Str(D,TEXT("id")),SC06Doc(D));}
  if(!Check(!Papers.IsEmpty()&&InputUnchanged(),TEXT("actual full acquired source inventory and immutable raw input pins")))return;
  FParse::Value(FCommandLine::Get(),TEXT("S47SC06Method="),Method);if(Method.IsEmpty())Method=TEXT("passive");if(!Check(Method==TEXT("passive")||Method==TEXT("active"),TEXT("explicit supported control method")))return;
  Expected=S47J::Clone(Prior);FS47SaveCard Card;Card.Chapter=TEXT("SC05 actual completed checkpoint");Card.Place=S47J::Str(Prior,TEXT("area"));Card.Clock=S47Text::ClockText(S47J::Num(S47J::Obj(Prior,TEXT("case")),TEXT("clock")),true);FString Error;
  const auto Seed=M->Flow->Saves.Save(1,TEXT("manual"),0,Expected,Card,Error);FS47SaveStore SeedReader(M->Flow->Saves.Dir);SeedReader.Init();const auto Imported=Seed?SeedReader.Load(Seed->Id):nullptr;
  if(!Check(Seed&&Seed->CaseId==1&&SC06Same(Imported,Prior),TEXT("only exact parent full state transferred into isolated case1/manual; independent reader: ")+Error))return;
  if(!ContinueDisk(TEXT("actual-sc05-input"),Imported))return;Next(1,.3);return;
 }
 if(!Pending.IsEmpty()&&!Restored())return;if(M->Mode!=US47Main::EMode::Play)return;auto* C=Act();
 switch(Stage)
 {
 case 1:if(!Check(C->S.SequenceVersion==1&&!C->S.F1&&M->Story->ChapterKey==TEXT("ch1")&&M->Story->Clock()>=3*3600+25*60,TEXT("actual completed SC05 begins fresh opt-in SC06 at03:25 without invented photographs")))return;if(!Store(TEXT("before-frame01")))return;Next(2);return;
 case 2:Path({{4.7,1.4},{4.8,.75}},3);return;
 case 3:if(!Walk())return;if(!C->S.bCamera){if(!Face(TEXT("fieldCamera")))return;Next(4);}else Next(5,0);return;
 case 4:if(!Use(TEXT("fieldCamera"))||!Check(C->S.bCamera,TEXT("ordinary camera collection retained")))return;Next(5,0);return;
 case 5:Path({{4.8,1.6},{5.1,1.6}},6);return;
 case 6:if(!Walk())return;if(!Face(TEXT("doorEast")))return;Next(7);return;
 case 7:if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Use(TEXT("doorEast")))return;Next(8,.7);return;
 case 8:Path({{7.3,1.6},{9.9,1.6},{9.9,-1.2},{10.7,-10},{10.7,-11.05}},9);return;
 case 9:if(!Walk())return;if(!Face(TEXT("motorBus")))return;Next(10);return;
 case 10:if(!Use(TEXT("motorBus")))return;M->UI->Close(true);if(!Check(C->S.bLog,TEXT("actual S-03 paper read, not a log flag fixture")))return;Path({{10.4,-11.6}},11);return;
 case 11:if(!Walk())return;Frame=1;FacePoint(C->S03Centre());C->ToggleCamera();Next(12,.4);return;
 case 12:if(!Photograph())return;Next(13,.8);return;
 case 13:if(!Store(TEXT("sealed-frame01")))return;Next(14);return;
 case 14:Path({{10.1,-1.2},{11.7,-1.2}},15);return;
 case 15:if(!Walk())return;if(!Face(TEXT("labDoor")))return;Next(16);return;
 case 16:if(!M->Web->IsDoorOpen(TEXT("labDoor"))&&!Use(TEXT("labDoor")))return;Next(17,.7);return;
 case 17:Path({{13.65,-1.2}},18);return;
 case 18:if(!Walk())return;if(!Face(TEXT("labDoor")))return;Next(19);return;
 case 19:if(M->Web->IsDoorOpen(TEXT("labDoor"))&&!Use(TEXT("labDoor")))return;Path({{15.1,-1.2},{15.1,-4.4}},20);return;
 case 20:if(!Walk())return;if(!Face(TEXT("wetBench")))return;Next(21);return;
 case 21:if(!Use(TEXT("wetBench")))return;Next(22);return;
 case 22:if(!Button(TEXT("LOAD SEALED FILM")))return;Next(Frame==1?23:32,2.5);return;
 case 23:if(!Check(C->S.Dev1==1,TEXT("real tank timer completed; unstable print ready for deliberate light failure")))return;Path({{15.1,-1.2},{13.65,-1.2}},24);return;
 case 24:if(!Walk())return;if(!Face(TEXT("labDoor")))return;Next(25);return;
 case 25:if(!Use(TEXT("labDoor")))return;Next(26,.3);return;
 case 26:if(!Check(C->S.Fog1==1&&C->S.Dev1==0&&C->S.F1.IsSet(),TEXT("actual opened door fogs paper and retains original negative")))return;if(!Store(TEXT("fogged-print-negative-kept")))return;Next(27);return;
 case 27:if(!Face(TEXT("labDoor")))return;Next(28);return;
 case 28:if(M->Web->IsDoorOpen(TEXT("labDoor"))&&!Use(TEXT("labDoor")))return;Path({{15.1,-1.2},{15.1,-4.4}},29);return;
 case 29:if(!Walk())return;if(!Face(TEXT("wetBench")))return;Next(30);return;
 case 30:if(!Use(TEXT("wetBench")))return;Next(31);return;
 case 31:if(!Button(TEXT("LOAD SEALED FILM")))return;Next(32,2.5);return;
 case 32:if(!Check((Frame==1?C->S.Dev1:C->S.Dev2)==1,TEXT("actual fresh-paper tank timer finishes"))||!Use(TEXT("wetBench")))return;Next(33);return;
 case 33:if(!Button(TEXT("TRANSFER CONTACT PRINT TO FIXER")))return;Next(34,1.8);return;
 case 34:if(!Check((Frame==1?C->S.Dev1:C->S.Dev2)==2,TEXT("actual fixer timer stabilizes print"))||!Use(TEXT("wetBench")))return;Next(35);return;
 case 35:if(!Button(TEXT("COLLECT AND EXAMINE PRINT")))return;Next(36);return;
 case 36:if(!MarkPrint(Frame))return;M->UI->Close(true);Path({{17.55,-3.0}},37);return;
 case 37:if(!Walk())return;if(!Face(TEXT("archive")))return;Next(38);return;
 case 38:if(!Use(TEXT("archive")))return;Next(39);return;
 case 39:
 {
  auto* B=C->Book();if(!Check(B!=nullptr,TEXT("production board rulebook present")))return;
  if(Frame==1)
  {
   auto Link=B->Connect(TEXT("frame01.detail"),TEXT("refsheet.b12"),false);auto Ref=B->Record(TEXT("P01"));
   auto Hyp=B->Connect(Method==TEXT("passive")?TEXT("hyp.light"):TEXT("hyp.drift"),Method==TEXT("passive")?TEXT("refsheet.lamp"):TEXT("refsheet.drive"),false);auto Recorded=B->Record(TEXT("P02"));
   if(!Check(Link&&Link->bOk&&Ref&&Ref->bOk&&Hyp&&Hyp->bOk&&Recorded&&Recorded->bOk&&C->S.bRef&&!C->S.Hyp.IsEmpty(),TEXT("actual P01/P02 board pins and recording identify B-12 and a testable cause")))return;M->UI->Close(true);if(!Store(TEXT("frame01-interpreted")))return;Next(40);return;
  }
  auto Retain=B->Connect(TEXT("frame01.extra"),TEXT("frame02.extra"),false);auto Observe=B->Connect(TEXT("observation.stripe"),TEXT("frame02.detail"),false);auto Result=B->Record(TEXT("P03"));
  if(!Check(Retain&&Retain->bOk&&Observe&&Observe->bOk&&Result&&Result->bOk&&C->S.bConcluded,TEXT("actual P03 compares both real photographs with direct physical observation")))return;M->UI->Close(true);Path({{17.2,-.55}},54);return;
 }
 case 40:Path({{15.1,-1.2},{13.65,-1.2}},41);return;
 case 41:if(!Walk())return;if(!Face(TEXT("labDoor")))return;Next(42);return;
 case 42:if(!M->Web->IsDoorOpen(TEXT("labDoor"))&&!Use(TEXT("labDoor")))return;Next(43,.7);return;
 case 43:Path({{11.7,-1.2},{9.9,-1.2},{9.2,-16.4}},44);return;
 case 44:if(!Walk())return;if(!Face(TEXT("b12Control")))return;Next(45);return;
 case 45:if(!Use(TEXT("b12Control")))return;Next(46);return;
 case 46:if(!Button(Method==TEXT("passive")?TEXT("PASSIVE / SHIELD THE LAMP"):TEXT("ACTIVE / DRIVE TO 042")))return;Next(47,2.1);return;
 case 47:if(!Check(C->S.Method==Method&&C->bSettled&&FMath::Abs(C->Yard->VaneObj.Rotation().Y-(Method==TEXT("passive")?-60.:42.)*PI/180)<1e-5,TEXT("selected physical control settles through production timer"))||!Use(TEXT("b12Control")))return;M->UI->Close(true);if(!Check(C->S.bObserved,TEXT("actual direct observation recorded at B-12")))return;Path({{10,-15}},48);return;
 case 48:if(!Walk())return;Frame=2;FacePoint(C->GetYard()->VaneObj.WorldPosition());C->ToggleCamera();Next(49,.4);return;
 case 49:if(!Photograph())return;Next(50,.8);return;
 case 50:if(!Store(TEXT("sealed-frame02")))return;Next(51);return;
 case 51:Path({{10.1,-1.2},{11.7,-1.2}},52);return;
 case 52:if(!Walk())return;if(!Face(TEXT("labDoor")))return;Next(53);return;
 case 53:if(!M->Web->IsDoorOpen(TEXT("labDoor"))&&!Use(TEXT("labDoor")))return;Next(17,.7);return;
 case 54:if(!Walk())return;if(!Face(TEXT("records")))return;Next(55);return;
 case 55:if(!Use(TEXT("records")))return;Next(56);return;
 case 56:if(!Button(TEXT("FILE PRINTS AND CLOSE THE LOCAL CASE")))return;Next(57,3);return;
 case 57:if(M->Story->IsCinematic())return;if(!Check(C->S.bFiled&&C->S.F1&&C->S.F2&&C->S.Fog1==1&&M->Story->Clock()>=3*3600+40*60&&M->Story->ChapterKey==TEXT("ch2"),TEXT("actual report completion advances to03:40 and ordinary Chapter2")))return;if(!Store(TEXT("sc06-complete"),true))return;Next(58);return;
 case 58:if(!Documents(true))return;Next(59);return;
 case 59:if(!Store(TEXT("ordinary-title-continue")))return;Next(60,.3);return;
 case 60:if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&TitleContinued&&TitleContinues==Stores&&Restores.Num()==Stores&&Photos.Num()==2&&Uses>=14&&Stores>=5&&Metres>60&&Samples>100,TEXT("both real photographs, fog/reprint, physical route and full disk/title continuation completed")))return;Finish(true,TEXT("actual B-12 control, two exposures and source-preserving continuation"));return;
 default:Finish(false,TEXT("unknown SC06 stage"));return;
 }
}
void FS47SC06Contract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47SC06Contract> Run;if(Owner.Get()!=Main){Owner=Main;Run=MakeShareable(new FS47SC06Contract(Main));}const auto KeepAlive=Run;if(KeepAlive)KeepAlive->Tick(Dt);
}
