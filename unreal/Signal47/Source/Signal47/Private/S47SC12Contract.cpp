// Opt-in SC12 fixture and production-action contract. No SC11 escape, human input or full-night claim.
#include "S47Chapter5.h"
#include "S47Chapter6.h"
#include "S47Diner.h"
#include "S47Drawings.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47NightContract.h"
#include "S47SC11.h"
#include "S47SC05.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47ThreatNavigation.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
 FString SC12Full(FString Path){Path=FPaths::ConvertRelativePathToFull(Path);FPaths::CollapseRelativeDirectories(Path);FPaths::NormalizeFilename(Path);return Path;}
 FString SC12Hash(const FString& Path)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return {};
  FSHAHash H;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),H.Hash);return H.ToString();
 }
 S47J::FObj SC12File(const FString& Path)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("path"),SC12Full(Path));S47J::Set(O,TEXT("bytes"),double(IFileManager::Get().FileSize(*Path)));S47J::Set(O,TEXT("sha1"),SC12Hash(Path));return O;
 }
 S47J::FObj SC12Read(const FString& Path){FString Text;return FFileHelper::LoadFileToString(Text,*Path)?S47J::Read(Text):S47J::FObj();}
 S47J::FObj SC12SourceDoc(const S47J::FObj& Saved)
 {
  if(!Saved)return nullptr;auto O=S47J::Clone(Saved);if(!O)return nullptr;
  if(S47J::Has(O,TEXT("ratio"))){if(S47J::Has(O,TEXT("paperRatio"))&&S47J::Num(O,TEXT("paperRatio"))!=S47J::Num(O,TEXT("ratio")))return nullptr;S47J::Set(O,TEXT("paperRatio"),S47J::Num(O,TEXT("ratio")));O->RemoveField(TEXT("ratio"));}
  return O;
 }
 S47J::FObj SC12Pointing(US47Main* M)
 {
  auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> A;
  for(int32 I=0;I<M->Web->DishCount();++I)A.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(M->Web->DishAz(I)),MakeShared<FJsonValueNumber>(M->Web->DishEl(I))}));
  O->SetArrayField(TEXT("pointing"),A);return O;
 }
}

class FS47SC12Contract : public TSharedFromThis<FS47SC12Contract>
{
 using FJ = S47J::FObj;
 TWeakObjectPtr<US47Main> Main;
 bool Done = false, Armed = false, ActualInput = false, SawDriving = false;
 int32 Stage = 0, Checks = 0, Restores = 0, RayUses = 0, Waypoint = 0;
 double Elapsed = 0, Wait = 0, WalkMetres = 0, Stalled = 0, Driven = 0, DriveOdo = 0;
 FString Root, Chain, InputPath, LastDriveLeg;
 FJ InputState, InputFile, InputResultFile, InitialPointing, CheckpointFile, DepartureFile;
 TMap<FString,FJ> OriginalDocs;
 FVector LastFoot = FVector::ZeroVector;
 TArray<FVector> Path;
 TArray<TSharedPtr<FJsonValue>> Evidence, Rays, Transitions;
 FS47Doc Third;
 TArray<FS47Doc> AcquiredSources;
 US47Main* M() const { return Main.Get(); }
 US47Chapter5* C() const { return M() && M()->Story ? Cast<US47Chapter5>(M()->Story->Chapter(TEXT("ch5"))) : nullptr; }
 bool Write(const FString& Name, const FJ& O) const { return FFileHelper::SaveStringToFile(S47J::Write(O, true), *(Root / Name)); }
 static bool Same(const FJ& A, const FJ& B) { return A && B && FJsonValue::CompareEqual(FJsonValueObject(A), FJsonValueObject(B)); }
 static FJ DocFields(const FS47Doc& D)
 {
  auto O = S47J::New(); S47J::Set(O,TEXT("id"),D.Id); S47J::Set(O,TEXT("title"),D.Title); S47J::Set(O,TEXT("kind"),D.Kind);
  S47J::Set(O,TEXT("stamp"),D.Stamp); S47J::Set(O,TEXT("page"),D.Page); S47J::Set(O,TEXT("transcript"),D.Transcript);
  S47J::Set(O,TEXT("image"),D.Image); S47J::Set(O,TEXT("paper"),D.Paper); S47J::Set(O,TEXT("paperRatio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;
  for (const auto& Sheet : D.Sheets)
  {
   auto Row = S47J::New(); S47J::Set(Row,TEXT("page"),Sheet.Page); S47J::Set(Row,TEXT("paper"),Sheet.Paper);
   S47J::Set(Row,TEXT("strip"),Sheet.Strip); S47J::Set(Row,TEXT("term"),Sheet.Term); Sheets.Add(MakeShared<FJsonValueObject>(Row));
  }
  O->SetArrayField(TEXT("sheets"),Sheets); return O;
 }
 TArray<TSharedPtr<FJsonValue>> CurrentSources() const
 {
  TArray<TSharedPtr<FJsonValue>> Rows;
  if (M() && M()->Story) for (const auto& Original : AcquiredSources)
   if (const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; })) Rows.Add(MakeShared<FJsonValueObject>(DocFields(*Actual)));
  return Rows;
 }
 bool SourceIntegrity(const FString& Context)
 {
  if(!ActualInput)return true;
  for(const auto& Entry:OriginalDocs)
  {
   const auto* D=M()->Story->Docs().FindByPredicate([&](const FS47Doc& Paper){return Paper.Id==Entry.Key;});
   if(!Check(D&&Same(DocFields(*D),Entry.Value),Context+TEXT(" all acquired original document fields/sheets preserved: ")+Entry.Key))return false;
  }
  for(const auto& Paper:AcquiredSources)
  {
   const auto* D=M()->Story->Docs().FindByPredicate([&](const FS47Doc& P){return P.Id==Paper.Id;});
   if(!Check(D&&Same(DocFields(*D),DocFields(Paper)),Context+TEXT(" actual SC11 source bytes, never factory replacements: ")+Paper.Id))return false;
  }
  return Check(M()->Web->DishCount()==27&&Same(InitialPointing,SC12Pointing(M())),Context+TEXT(" all27 actual encoders unchanged"));
 }
 bool AdoptInput()
 {
  InputPath=SC12Full(InputPath);const FString ResultPath=FPaths::GetPath(InputPath)/TEXT("result.json");
  InputState=SC12Read(InputPath);const auto Prior=SC12Read(ResultPath),Pin=S47J::Obj(Prior,TEXT("checkpoint_file"));
  InputFile=SC12File(InputPath);InputResultFile=SC12File(ResultPath);Chain=S47J::Str(Prior,TEXT("chain_id"));
  if(!Check(FPaths::GetCleanFilename(InputPath)==TEXT("sc11-complete.disk.json")&&InputState&&Prior&&Pin
   &&S47J::Str(Prior,TEXT("status"))==TEXT("PASS")&&S47J::Str(Prior,TEXT("scenario"))==TEXT("SC11")
   &&S47J::Str(Prior,TEXT("mode"))==TEXT("actual-checkpoint")&&!Chain.IsEmpty()
   &&SC12Full(S47J::Str(Pin,TEXT("path")))==InputPath&&S47J::Num(Pin,TEXT("bytes"))==S47J::Num(InputFile,TEXT("bytes"))&&S47J::Num(InputFile,TEXT("bytes"))>0
   &&S47J::Str(Pin,TEXT("sha1")).Equals(S47J::Str(InputFile,TEXT("sha1")),ESearchCase::IgnoreCase),TEXT("actual SC11 complete disk/PASS/scenario/actual-mode/chain and exact path/bytes/SHA1; fixture output rejected")))return false;
  const auto Spine=S47J::Obj(InputState,TEXT("spine")),Siege=S47J::Obj(Spine,TEXT("sc11d")),Case=S47J::Obj(InputState,TEXT("case")),Ch5=S47J::Obj(Case,TEXT("ch5")),R=S47J::Obj(Ch5,TEXT("sc12"));
  const int32 Version=S47J::Int(Spine,TEXT("v"));
  if(!Check(S47J::Int(InputState,TEXT("timelineVersion"))==2&&(Version==1||Version==2)&&FS47SC05::CanAdoptSC04(S47J::Obj(Spine,TEXT("sc04d")))
   &&S47J::Int(Spine,TEXT("sc04"))==2&&S47J::Int(Spine,TEXT("sc08"))==2&&S47J::Int(Spine,TEXT("sc11"))==2&&S47J::Int(Spine,TEXT("sc12Route"))==1
   &&S47J::Int(Siege,TEXT("v"))==1&&S47J::Int(Siege,TEXT("step"))==int32(FS47SC11::EStep::Done)
   &&S47J::Bool(Siege,TEXT("receiver"))&&S47J::Bool(Siege,TEXT("event"))&&S47J::Bool(Siege,TEXT("hondo"))&&S47J::Bool(Siege,TEXT("site11"))
   &&S47J::Bool(Siege,TEXT("checkedLocalLight"))&&S47J::Bool(Siege,TEXT("observedRoom"))&&S47J::Bool(Siege,TEXT("ctx_hr_h07_escape_complete"))&&!S47J::Str(Siege,TEXT("escapedAt")).IsEmpty()
   &&S47J::Str(InputState,TEXT("checkpoint"))==TEXT("chapter5")&&S47J::Str(InputState,TEXT("area"))==TEXT("saro")&&S47J::Str(Ch5,TEXT("stage"))==TEXT("to-truck")
   &&S47J::Int(R,TEXT("v"))==1&&!S47J::Bool(Ch5,TEXT("arrived"))&&!S47J::Bool(R,TEXT("laid"))&&!S47J::Bool(R,TEXT("warningSent"))&&!S47J::Bool(R,TEXT("courier104"))
   &&!S47J::Bool(R,TEXT("compared"))&&S47J::Int(R,TEXT("read"))==0&&S47J::Int(R,TEXT("checks"))==0&&!S47J::Bool(R,TEXT("routeReadback"))
   &&S47J::Num(Case,TEXT("clock"))>=5*3600+10*60&&S47J::Num(Case,TEXT("clock"))<5*3600+15*60,
   TEXT("source-qualified v1/v2 actual SC11 escape with physical light/room/sources, natural C5/to-truck before SC12 actions or sheriff")))return false;
  TArray<uint8> Bytes,ResultBytes;
  if(!Check(FFileHelper::LoadFileToArray(Bytes,*InputPath)&&FFileHelper::LoadFileToArray(ResultBytes,*ResultPath)
   &&FFileHelper::SaveArrayToFile(Bytes,*(Root/TEXT("sc11-input-original.disk.json")))&&FFileHelper::SaveArrayToFile(ResultBytes,*(Root/TEXT("sc11-input-result-original.json")))
   &&SC12Hash(Root/TEXT("sc11-input-original.disk.json"))==SC12Hash(InputPath)&&SC12Hash(Root/TEXT("sc11-input-result-original.json"))==SC12Hash(ResultPath),TEXT("preserve actual SC11 input and sibling PASS byteoriginals in private proof; originals never modified")))return false;
  FS47SaveCard Card;Card.Chapter=TEXT("Actual SC11 completed escape");Card.Place=TEXT("SARO truck pad");Card.Clock=TEXT("actual saved time");FString Error;
  const auto Meta=M()->Flow->Saves.Save(1,TEXT("manual"),0,InputState,Card,Error);FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();
  if(!Check(Meta&&Same(Reader.Load(Meta->Id),InputState),TEXT("fresh case1 disk/independent reader stores unchanged full SC11 cohort: ")+Error))return false;
  M()->Flow->ToTitle();FS47SaveStore TitleReader(M()->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),Live=M()->Flow->Saves.ContinueSave();
  if(!Check(M()->Mode==US47Main::EMode::Title&&Selected&&Live&&Selected->Id==Meta->Id&&Live->Id==Meta->Id&&Same(TitleReader.Load(Selected->Id),InputState)
   &&M()->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title/new reader/Continue actually restores the SC11 cohort")))return false;
  ++Restores;const auto Actual=M()->Flow->GameState(),Pose=S47J::Obj(InputState,TEXT("pose")),ActualCase=S47J::Obj(Actual,TEXT("case")),ActualSpine=S47J::Obj(Actual,TEXT("spine"));
  if(!Check(M()->Flow->CaseId==1&&!M()->Flow->bDevRun&&M()->Story->bEveningTimeline&&C()->bActive&&C()->RevisedDiner()&&Same(C()->S.Save(),Ch5)
   &&M()->World->Area==TEXT("saro")&&!M()->World->bDriving&&!M()->World->bCommuteVehicle&&M()->World->TruckAt==TEXT("saro")&&Pose
   &&M()->Player->Walkable(M()->Player->Pos.X,M()->Player->Pos.Z)&&M()->Story->Clock()==S47J::Num(Case,TEXT("clock"))
   &&FMath::Abs(M()->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.0001&&FMath::Abs(M()->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.0001
   &&FMath::Abs(M()->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.00001&&FMath::Abs(M()->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.00001,
   TEXT("actual case1 C5/clock/walkable saved SARO pose and original service truck retained; no placement or clock fixture")))return false;
  const auto Match=[&](const FJ& A,const FJ& B,const TCHAR* Key){const auto X=A?A->TryGetField(Key):nullptr,Y=B?B->TryGetField(Key):nullptr;return(!X&&!Y)||(X&&Y&&FJsonValue::CompareEqual(*X,*Y));};
  for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4")})if(!Check(Match(Case,ActualCase,Key),FString(TEXT("SC11 full prior chapter state retained: "))+Key))return false;
  for(const TCHAR* Key:{TEXT("sc04d"),TEXT("sc05d"),TEXT("sc06d"),TEXT("sc07d"),TEXT("sc08d"),TEXT("sc11d"),TEXT("sc04"),TEXT("sc08"),TEXT("sc11"),TEXT("sc12Route")})
   if(!Check(Match(Spine,ActualSpine,Key),FString(TEXT("SC11 full gate/source payload retained: "))+Key))return false;
  for(const TCHAR* Key:{TEXT("doors"),TEXT("horror"),TEXT("observing"),TEXT("substation")})if(!Check(Match(InputState,Actual,Key),FString(TEXT("SC11 protected actual production field: "))+Key))return false;
  AcquiredSources=C()->S.SC12.SourcePapers;
  if(!Check(AcquiredSources.Num()==3&&C()->HasSC12Sources()&&!C()->SourcesVerified()&&!C()->ReadyForRoad(),TEXT("three actual owned and serialized independent sources exist before read/comparison; none manufactured")))return false;
  const TCHAR* Ids[]={TEXT("HR_SARO_ORIGINAL"),TEXT("HR_HONDO_REMOTE"),TEXT("HR_SITE11_REMOTE")};
  const TCHAR* SourceIds[]={TEXT("SARO07-860414-RX01"),TEXT("HONDO-860414-LOG01"),TEXT("SITE11-860414-TR01")};
  for(int32 I=0;I<3;++I)if(!Check(AcquiredSources.ContainsByPredicate([&](const FS47Doc& D){return D.Id==Ids[I]&&D.Page.Contains(FString(TEXT("SOURCE ID / "))+SourceIds[I],ESearchCase::CaseSensitive);}),TEXT("actual SC11 source ID preserved: ")+FString(SourceIds[I])))return false;
  const auto Array=S47J::Obj(Spine,TEXT("sc08d"));InitialPointing=S47J::New();InitialPointing->SetArrayField(TEXT("pointing"),S47J::Arr(Array,TEXT("pointings")));
  if(!Check(S47J::Int(Array,TEXT("v"))==1&&S47J::Bool(Array,TEXT("escaped"))&&S47J::Int(Array,TEXT("plays"))>=2
   &&S47J::Arr(Array,TEXT("pointings")).Num()==27&&Same(InitialPointing,SC12Pointing(M())),TEXT("all27 loaded actual encoders match the completed SC08 source snapshot exactly")))return false;
  const auto SavedSources=S47J::Arr(Array,TEXT("sources"));if(!Check(!SavedSources.IsEmpty(),TEXT("actual preceding SC08 source inventory exists in SC11 checkpoint")))return false;
  for(const auto& V:SavedSources)
  {
   const auto Paper=V&&V->Type==EJson::Object?SC12SourceDoc(V->AsObject()):FJ();const auto* ActualPaper=Paper?M()->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==S47J::Str(Paper,TEXT("id"));}):nullptr;
   if(!Check(ActualPaper&&Same(Paper,DocFields(*ActualPaper)),TEXT("preceding serialized source/sheets match actual input paper: ")+S47J::Str(Paper,TEXT("id"))))return false;
  }
  for(const auto& D:M()->Story->Docs())OriginalDocs.Add(D.Id,DocFields(D));
  return SourceIntegrity(TEXT("SC11 input"))&&Snapshot(TEXT("actual-sc11-after-title-continue"));
 }
 bool GoItem(const FString& Id)
 {
  const auto* Item=M()->Inter->Get(Id);const auto* WebItem=M()->Web->Item(Id);const FS47Obj O=Item&&Item->Object?Item->Object:WebItem?M()->Web->ObjAt(WebItem->Node):FS47Obj();
  const auto Bounds=O.WorldBox();if(!Check(O&&Bounds.IsValid,TEXT("original physical target for actual approach: ")+Id))return false;
  const double Range=Item&&Item->Range>0?Item->Range:WebItem?WebItem->Range:3.2;const FVector Center=Bounds.GetCenter();const FVector2D From(M()->Player->Pos.X,M()->Player->Pos.Z);
  TArray<FVector2D> Seeds;for(const auto& N:FS47SC11::Nodes())if(FVector2D::Distance(From,N.At)<150)Seeds.Add(N.At);
  // Existing doorway/aisle vertices, transformed by the actual retained diner group.
  if(M()->World->Area==TEXT("diner"))for(const FVector L:{FVector(6.5,0,6.6),FVector(6.5,0,2.7),FVector(3.6,0,2.7),FVector(-1.4,0,3.05)})
  {const auto P=C()->Diner()->Group.LocalToWorld(L);Seeds.Add({P.X,P.Z});}
  TArray<FVector2D> Best;double Cost=1e100;
  for(double R:{1.1,1.6,2.1,2.6})for(int32 I=0;I<16;++I)
  {
   const double A=UE_DOUBLE_PI*I/8;const FVector2D To(Center.X+R*FMath::Cos(A),Center.Z+R*FMath::Sin(A));if(!M()->Player->Walkable(To.X,To.Y))continue;
   const FVector Eye(To.X,M()->Player->FloorY+M()->Player->Eye,To.Y),D=Center-Eye;
   if(D.Size()>Range-.08||M()->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)continue;
   auto Found=FS47ThreatNavigation::FindPath(From,To,Seeds,M()->Web->Colliders(),M()->Player->Radius,[&](const FVector2D& X,const FVector2D& Y)
   {const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(X,Y)/.05));for(int32 J=0;J<=N;++J){const auto P=FMath::Lerp(X,Y,double(J)/N);if(!M()->Player->Walkable(P.X,P.Y))return false;}return true;});
   double Length=0;auto Previous=From;for(const auto& P:Found){Length+=FVector2D::Distance(Previous,P);Previous=P;}
   if(!Found.IsEmpty()&&Length<Cost){Best=MoveTemp(Found);Cost=Length;}
  }
  if(!Check(!Best.IsEmpty(),TEXT("complete bounded native walkable route to actual target; no Place/endpoint fixture: ")+Id))return false;
  Path.Reset();for(const auto& P:Best)Path.Add(FVector(P.X,0,P.Y));Waypoint=0;LastFoot=M()->Player->Pos;Stalled=Elapsed;return true;
 }
 bool ExportBoundary()
 {
  if(!SourceIntegrity(TEXT("SC12 complete")))return false;
  FJ Proof;FString Error;const bool Cycled=S47NightContract::RoundTrip(M(),Root,TEXT("sc12-complete"),Proof,Error);
  if(!Check(Cycled,TEXT("safe actual05:20 output normal manual/new reader/title/Continue: ")+Error))return false;
  ++Restores;const FString Complete=Root/TEXT("sc12-complete.disk.json"),Departure=Root/TEXT("departure-checkpoint.disk.json");TArray<uint8> Bytes;
  if(!Check(!IFileManager::Get().FileExists(*Departure)&&FFileHelper::LoadFileToArray(Bytes,*Complete)&&FFileHelper::SaveArrayToFile(Bytes,*Departure)
   &&SC12Hash(Complete)==SC12Hash(Departure),TEXT("SC13 original departure filename and common sc12-complete file are byteidentical, not independently reserialized")))return false;
  CheckpointFile=SC12File(Complete);DepartureFile=SC12File(Departure);
  const auto Disk=SC12Read(Complete),SavedCase=S47J::Obj(Disk,TEXT("case")),R=S47J::Obj(S47J::Obj(SavedCase,TEXT("ch5")),TEXT("sc12"));
  return Check(S47J::Str(Disk,TEXT("checkpoint"))==TEXT("chapter5")&&S47J::Str(Disk,TEXT("area"))==TEXT("diner")
   &&S47J::Str(S47J::Obj(SavedCase,TEXT("ch5")),TEXT("stage"))==TEXT("road")&&S47J::Bool(R,TEXT("routeReadback"))&&S47J::Num(SavedCase,TEXT("clock"))==S47Timeline::Departure
   &&C()->bActive&&C()->ReadyForRoad()&&!M()->Story->Begun.Contains(TEXT("ch6"))&&M()->World->TruckAt==TEXT("diner")
   &&Same(InputFile,SC12File(InputPath))&&Same(InputResultFile,SC12File(FPaths::GetPath(InputPath)/TEXT("result.json"))),TEXT("safe ordinary post-Continue departure retains full actual source/receipt/P13 facts and immutable original SC11 input/PASS"))&&SourceIntegrity(TEXT("SC12 output after Continue"));
 }
 void Result(bool Pass, const FString& Why)
 {
  const auto KeepAlive = AsShared(); if (Done) return; Done = true;
  if (M() && M()->Player) M()->Player->TestMove.Reset();
  if(M()&&M()->World)M()->World->Autopilot.Reset();
  auto O = S47J::New(); S47J::Set(O, TEXT("status"), Pass ? TEXT("PASS") : TEXT("FAIL")); S47J::Set(O, TEXT("reason"), Why);
  S47J::Set(O,TEXT("scenario"),TEXT("SC12"));S47J::Set(O,TEXT("mode"),ActualInput?TEXT("actual-checkpoint"):TEXT("named-prerequisite-fixture"));S47J::Set(O,TEXT("chain_id"),Chain);
  S47J::Set(O,TEXT("process_id"),double(FPlatformProcess::GetCurrentProcessId()));S47J::Set(O,TEXT("checkpoint_file"),CheckpointFile);S47J::Set(O,TEXT("departure_checkpoint_file"),DepartureFile);
  auto Prior=S47J::New();S47J::Set(Prior,TEXT("checkpoint"),InputFile);S47J::Set(Prior,TEXT("result"),InputResultFile);S47J::Set(O,TEXT("prerequisite"),Prior);
  S47J::Set(O,TEXT("final_state"),M()&&M()->Flow?M()->Flow->GameState():FJ());S47J::Set(O,TEXT("driveMetres"),Driven);O->SetArrayField(TEXT("transitions"),Transitions);
  S47J::Set(O, TEXT("stage"), Stage); S47J::Set(O, TEXT("checks"), Checks); S47J::Set(O, TEXT("diskFlowRestores"), Restores);
  S47J::Set(O, TEXT("ordinaryRayUses"), RayUses); S47J::Set(O, TEXT("walkMetres"), WalkMetres); S47J::Set(O, TEXT("seconds"), Elapsed);
  S47J::Set(O, TEXT("lastReadback"), C() ? C()->SC12Readback() : FJ()); O->SetArrayField(TEXT("checksExecuted"), Evidence); O->SetArrayField(TEXT("rays"), Rays);
  O->SetArrayField(TEXT("actualHostSourcePapers"),CurrentSources());
  S47J::Set(O, TEXT("scope"), TEXT("Explicit isolated chapter5/SC11-secured-source fixture using the actual FS47SC11 document factories, not a played SC11 escape. One declared diner/foot start. Ordinary Walker between booth, phone and dated patrol sheet; actual camera/focus/ray and Player.OnUse. Conversation choices and tracing motion use the exact production callbacks, not human Slate input. Incomplete sources, early acknowledgements, interrupted calls, map errors and full disk/title/Continue at each stable boundary. All current Host.Docs source fields are independently compared with acquired fixture pages before and after Continue. Final service-truck choice is a labelled production API action, not a measured road drive. No screenshots, listening, full-night, packaging or installation acceptance."));
  if(ActualInput)S47J::Set(O,TEXT("scope"),TEXT("Actual hash-bound SC11 PASS/full case1 checkpoint with physical escape and its three complete owned source papers. Native Walker to the retained SARO truck, actual camera/focus/OnUse and ordinary production diner trip/park, then physical diner approach. No test Place/SetClock/prior-fact/doc factory. Native diner routes, text-only Ward/readback/patrol/P13/order actions and genuine manual/fresh-reader/title/Continue, full source payloads and27 encoders protected. Normal chosen departure saves the safe05:20 road checkpoint before SC13; sc12-complete and departure-checkpoint are byteidentical. Prior scenarios are linked actual evidence, not replayed here. API controls are not human input or listening/visual/full-night/package/install acceptance."));
  const bool Written = Root.IsEmpty() || Write(TEXT("result.json"), O);
  if (Pass && Written) { UE_LOG(LogTemp, Display, TEXT("S47: spine contract sc12 PASS COMPLETE: %d checks, %d restores, %d rays, %.3fm"), Checks, Restores, RayUses, WalkMetres); FPlatformMisc::RequestExitWithStatus(false, 0); }
  else { UE_LOG(LogTemp, Error, TEXT("S47: spine contract sc12 FAIL stage %d: %s"), Stage, *Why); FPlatformMisc::RequestExitWithStatus(false, 1); }
 }
 bool Check(bool Good, const FString& Why)
 {
  ++Checks; auto O = S47J::New(); S47J::Set(O, TEXT("pass"), Good); S47J::Set(O, TEXT("detail"), Why); S47J::Set(O, TEXT("stage"), Stage); Evidence.Add(MakeShared<FJsonValueObject>(O));
  if (!Good) { Result(false, Why); return false; } return true;
 }
 void Next(int32 S, double Delay = .25) { Stage = S; Wait = Elapsed + Delay; }
 bool Snapshot(const FString& Name)
 {
  const auto R = C()->SC12Readback();
  S47J::Set(R, TEXT("name"), Name); S47J::Set(R, TEXT("area"), M()->World->Area);
  S47J::Set(R, TEXT("case"), M()->Flow->ComposeCase());
  if(ActualInput){S47J::Set(R,TEXT("full_actual_game_state"),M()->Flow->GameState());S47J::Set(R,TEXT("actual_27_encoders"),SC12Pointing(M()));}
  R->SetArrayField(TEXT("actualHostSourcePapers"),CurrentSources());
  if (!Write(Name + TEXT(".json"), R)) { Result(false, TEXT("readback write failed: ") + Name); return false; } return true;
 }
 bool Choice(const FString& Id)
 {
  const auto View = C()->View();
  if (!Check(View.Choices.ContainsByPredicate([&](const S47Panels::FTalkChoice& X) { return X.Id == Id; }), TEXT("production panel offers choice: ") + Id)) return false;
  return Check(C()->SC12Choice(Id), TEXT("same guarded production choice callback: ") + Id);
 }
 void Close() { if (M()->UI) M()->UI->Close(true); }
 bool Restore(const FString& Name)
 {
  // Called only after closing the modal; this is the complete production case, never a hand-made ch5 restore.
  if (!Check(!M()->UI->Modal() && M()->Flow->GameSaveBlock().IsEmpty(), TEXT("normal stable save boundary: ") + Name)) return false;
  const auto Before = M()->Flow->GameState(), State5 = S47J::Obj(S47J::Obj(Before, TEXT("case")), TEXT("ch5"));
  const double Clock = M()->Story->Clock(); const FVector Foot = M()->Player->Pos;
  TArray<FS47Doc> BeforePapers;
  for (const auto& Original : AcquiredSources)
  {
   const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   if (!Actual) { if (!Check(Stage == 1 && Original.Id == Third.Id, TEXT("only the declared missing-third-source cohort may omit a paper: ") + Original.Id)) return false; continue; }
   if (!Check(Same(DocFields(*Actual),DocFields(Original)), TEXT("actual acquired Host.Docs payload before disk: ") + Original.Id + TEXT(" / ") + Name)) return false;
   BeforePapers.Add(*Actual);
  }
  if (!Check(Before && State5 && S47J::Str(Before, TEXT("checkpoint")) == TEXT("chapter5"), TEXT("actual complete chapter5 snapshot before disk: ") + Name)) return false;
  if(ActualInput)
  {
   if(!SourceIntegrity(TEXT("before ")+Name))return false;
   FJ Proof;FString Error;const bool Cycled=S47NightContract::RoundTrip(M(),Root,Name,Proof,Error);
   if(!Check(Cycled,TEXT("actual normal manual/fresh-reader/title/Continue: ")+Name+TEXT(" / ")+Error))return false;
   ++Restores;
   if(!Check(C()&&Same(C()->S.Save(),State5)&&C()->PendingReadback==0,TEXT("actual full C5 state and cleared live acknowledgement after Continue: ")+Name)||!SourceIntegrity(TEXT("after ")+Name))return false;
   const auto Now=M()->Flow->GameState(),BeforeCase=S47J::Obj(Before,TEXT("case")),NowCase=S47J::Obj(Now,TEXT("case")),BeforeSpine=S47J::Obj(Before,TEXT("spine")),NowSpine=S47J::Obj(Now,TEXT("spine"));
   const auto Match=[&](const FJ& A,const FJ& B,const TCHAR* Key){const auto X=A?A->TryGetField(Key):nullptr,Y=B?B->TryGetField(Key):nullptr;return(!X&&!Y)||(X&&Y&&FJsonValue::CompareEqual(*X,*Y));};
   for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4")})if(!Check(Match(BeforeCase,NowCase,Key),Name+TEXT(" full previous chapter bytes: ")+Key))return false;
   for(const TCHAR* Key:{TEXT("sc04d"),TEXT("sc05d"),TEXT("sc06d"),TEXT("sc07d"),TEXT("sc08d"),TEXT("sc11d")})if(!Check(Match(BeforeSpine,NowSpine,Key),Name+TEXT(" full previous Spine source bytes: ")+Key))return false;
   return Snapshot(Name+TEXT("-continued"));
  }
  FS47SaveStore Writer(Root / Name / TEXT("case1")); Writer.Init(); FS47SaveCard Card; Card.Chapter = M()->Story->ChapterTitle(); Card.Place = M()->Flow->PlaceName(); Card.Clock = M()->Story->ClockText(false); Card.Playtime = M()->Playtime;
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, Before, Card, Error);
  if (!Check(Meta.IsSet(), TEXT("isolated complete disk save: ") + Name + TEXT(" ") + Error)) return false;
  FS47SaveStore Reader(Writer.Dir); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
  if (!Check(Same(Before, Disk), TEXT("fresh reader equals complete written state: ") + Name)) return false;
  if (!Write(Name + TEXT(".disk.json"), Disk)) { Result(false, TEXT("disk evidence failed")); return false; }
  M()->Flow->Saves = FS47SaveStore(Writer.Dir); M()->Flow->Saves.Init(); M()->Flow->ToTitle();
  if (!Check(M()->Mode == US47Main::EMode::Title, TEXT("ordinary title reached: ") + Name)) return false;
  M()->Flow->OpenSave(*Meta); ++Restores;
  if (!Check(C() && Same(C()->S.Save(), State5) && FMath::Abs(M()->Story->Clock() - Clock) < .001 && M()->World->Area == TEXT("diner") && M()->Player->Pos.Equals(Foot, .015) && C()->PendingReadback == 0,
   TEXT("full Flow Continue keeps complete ch5/clock/area/pose; no live pending acknowledgement survives: ") + Name)) return false;
  for (const auto& Original : AcquiredSources)
  {
   const auto* Had = BeforePapers.FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   if (!Check((Had && Actual && Same(DocFields(*Actual),DocFields(*Had))) || (!Had && !Actual),
    TEXT("full Flow keeps actual Host.Docs fields, not replacement templates: ") + Original.Id + TEXT(" / ") + Name)) return false;
  }
  return Snapshot(Name + TEXT("-continued"));
 }
 void WalkTo(const TArray<FVector>& Local)
 {
  Path.Reset(); for (const FVector& P : Local) Path.Add(C()->Diner()->Group.LocalToWorld(P));
  Waypoint = 0; LastFoot = M()->Player->Pos; Stalled = Elapsed;
 }
 bool Walk(double Dt)
 {
  auto* Player = M()->Player.Get(); const double Moved = FVector2D(Player->Pos.X - LastFoot.X, Player->Pos.Z - LastFoot.Z).Size();
  if (Moved > FMath::Max(.5, Dt * 4 + .1)) { Result(false, TEXT("Walker discontinuity; no placement recovery")); return false; }
  if(ActualInput&&!Player->Walkable(Player->Pos.X,Player->Pos.Z)){Result(false,TEXT("actual predecessor route left native walkable land"));return false;}
  WalkMetres += Moved; LastFoot = Player->Pos; if (Moved > .002) Stalled = Elapsed;
  if (Elapsed - Stalled > 12) { Result(false, TEXT("ordinary diner Walker stalled")); return false; }
  if (Waypoint >= Path.Num()) { Player->TestMove.Reset(); return true; }
  const FVector D = Path[Waypoint] - Player->Pos; const double Distance = FVector2D(D.X, D.Z).Size();
  if (Distance < .085) { ++Waypoint; Player->TestMove.Reset(); return false; }
  Player->Yaw = FMath::Atan2(-D.X, -D.Z); Player->Pitch = -.08; Player->bTestRun = false; Player->TestMove = FVector2D(0, -FMath::Min(1., Distance / .5)); return false;
 }
 FVector Target(const FString& Id) const
 {
  if(Id==TEXT("truck")){const auto* Item=M()->Inter->Get(Id);return Item&&Item->Object?Item->Object.WorldBox().GetCenter():FVector::ZeroVector;}
  if (Id == TEXT("sc12:patrol")) return C()->SC12Object(TEXT("patrol")).WorldPosition();
  return C()->Diner()->Proxy(Id.RightChop(6)).WorldPosition();
 }
 void Look(const FString& Id)
 {
  const FVector D = Target(Id) - S47Three::ToThree(M()->Player->EyeUE());
  M()->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M()->Player->Pitch = FMath::Atan2(D.Y, FVector2D(D.X, D.Z).Size());
 }
 bool Use(const FString& Id)
 {
  const FVector Eye = M()->Player->EyeUE(), Dir = M()->Player->ForwardUE(); const FString Picked = M()->Inter->Update(Eye, Dir);
  auto R = S47J::New(); S47J::Set(R, TEXT("expected"), Id); S47J::Set(R, TEXT("picked"), Picked); S47J::Set(R, TEXT("focus"), M()->Player->FocusedId());
  S47J::Set(R, TEXT("eyeUE"), TArray<double>{Eye.X, Eye.Y, Eye.Z}); S47J::Set(R, TEXT("clock"), M()->Story->Clock()); Rays.Add(MakeShared<FJsonValueObject>(R));
  if (!Check(Picked == Id && M()->Player->FocusedId() == Id && bool(M()->Player->OnUse) && !M()->UI->Modal(), TEXT("actual camera/focus/ray then ordinary Player.OnUse: ") + Id)) return false;
  M()->Player->OnUse(Picked); ++RayUses; return true;
 }
public:
 explicit FS47SC12Contract(US47Main* InM) : Main(InM) {}
 void Tick(double Dt)
 {
  if (Done) return;
  if (!M() || !M()->Story || !M()->Flow || !M()->World || !M()->Player || !M()->Web || !M()->UI || !M()->Inter || !M()->Horror || !M()->Spine || !C()) { Result(false, TEXT("missing actual SC12 runtime dependency")); return; }
  Elapsed += Dt;
  if (Elapsed > (ActualInput?900:300)) { Result(false, TEXT("SC12 contract timeout; actual SARO→diner path has explicit900s budget")); return; }
  if (S47Startup::Blocking() || M()->Flow->bStarting || M()->World->bBusy || Elapsed < Wait) return;
  if (Stage == 0)
  {
   if (Elapsed < 3 || !C()->Diner()) return;
   FString UserDir; const bool HasUserDir = FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
   ActualInput=FParse::Value(FCommandLine::Get(),TEXT("S47SC11Checkpoint="),InputPath);FString Jump,Mode;FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);
   const bool NoParentTraversal = !UserDir.Contains(TEXT("..")); UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::NormalizeDirectoryName(UserDir);
   if (!Check(HasUserDir && NoParentTraversal && UserDir.StartsWith(TEXT("/tmp/"))
    && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
    && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M()->Flow->Saves.Dir),UserDir)
    && M()->Flow->Saves.List().IsEmpty() && M()->Flow->bDevRun && M()->Flow->CaseId == 0 && Mode==TEXT("sc12")
    &&(ActualInput?Jump.IsEmpty():M()->Story->bEveningTimeline&&M()->Story->ChapterKey==TEXT("ch5"))
    &&!IFileManager::Get().DirectoryExists(*(FPaths::ProjectSavedDir()/TEXT("SpineSC12Contract"))),
    TEXT("fresh isolated /tmp profile/Saved/SaveStore before writes; actual input noJump, named legacy chapter5 fixture retained"))) return;
   for(const TCHAR* Flag:{TEXT("S47Ch5Drive"),TEXT("S47Ch4Drive"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract"),TEXT("S47RadioWomanContract"),TEXT("S47ProductionViews"),TEXT("S47Drive"),TEXT("S47Auto"),TEXT("S47WalkFrom"),TEXT("S47Place"),TEXT("S47EveningBootstrap"),TEXT("S47SC10Checkpoint")})
   {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Result(false,TEXT("independent runtime/fixture driver also enabled: ")+FString(Flag));return;}}
   Root = FPaths::ProjectSavedDir() / TEXT("SpineSC12Contract") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
   if (!IFileManager::Get().MakeDirectory(*Root, true)) { Result(false, TEXT("fresh evidence directory unavailable")); return; }
   Armed = true;
   if(ActualInput){if(AdoptInput()&&GoItem(TEXT("truck")))Next(201,0);return;}
   // Prior scenarios and their secured papers are explicitly fixture data here, never a new escape assertion.
   M()->Spine->Begin(S47J::Read(TEXT("{\"v\":1,\"sc04\":2,\"sc08\":2,\"sc11\":2,\"sc12Route\":1}")), false);
   C()->Begin(nullptr); M()->World->PlaceTruck(TEXT("diner")); M()->World->GoDiner(TEXT("stool"));
   Next(100,.7); return;
  }
  if (Stage == 100)
  {
   if (!Check(M()->World->Area == TEXT("diner"), TEXT("declared diner transition actually completed before any foot placement"))) return;
   C()->ArrivedDiner();
   // These are the same factories the actual SC11 uses. The complete local source is an explicit
   // prerequisite fixture; calling this factory is never credited as playing its escape.
   const auto Local = FS47SC11::ReceiverDoc(true,TEXT("05:10:00 / SC12 PREREQUISITE FIXTURE"));
   const auto Hondo = FS47SC11::HondoDoc(); Third = FS47SC11::Site11Doc(); AcquiredSources = {Local,Hondo,Third};
   if (!Check(Local.Page.Contains(TEXT("ctx_hr_h07_escape_complete / WORLD ESCAPE RECORDED"))
    && !Hondo.Page.IsEmpty() && !Third.Page.IsEmpty(), TEXT("actual SC11 document factories supply the explicit secured-source fixture; no new escape credit"))) return;
   M()->Story->AddDoc(Local); M()->Story->AddDoc(Hondo);
   M()->Story->DocsMutable().RemoveAll([](const FS47Doc& D) { return D.Id == TEXT("HR_SITE11_REMOTE"); });
   const FVector At = C()->Diner()->Group.LocalToWorld(FVector(-.8, 0, 3.05));
   if (!Check(M()->Player->Walkable(At.X, At.Z), TEXT("one declared diner-aisle foot start is actually walkable"))) return;
   M()->Player->Place(At.X, At.Z, 0); Look(TEXT("diner:booth"));
   if (!Check(C()->RevisedDiner() && M()->Story->Clock() >= S47Timeline::Chapter5(true) && M()->Story->Clock() < 5 * 3600 + 15 * 60 && !C()->S.SC12.bDeputyHere && !C()->ReadyForRoad(), TEXT("new SC12 starts05:12 before sheriff05:15 and cannot depart"))) return;
   Next(1, .5); return;
  }
  if (!Armed) return;
  if(Stage==201){if(Walk(Dt)){Look(TEXT("truck"));Next(202,.35);}return;}
  if(Stage==202)
  {
   if(!Check(C()->S.Stage==TEXT("to-truck")&&C()->TruckTrip(TEXT("saro")).IsSet()&&M()->World->Area==TEXT("saro")&&M()->World->TruckAt==TEXT("saro")&&!M()->World->bDriving&&!M()->World->bCommuteVehicle,
    TEXT("actual SC11 escaped cohort reaches retained SARO service truck before ordinary C5 diner choice")))return;
   DriveOdo=M()->World->Odometer();SawDriving=false;LastDriveLeg.Reset();M()->World->Autopilot=20;Stalled=Elapsed;
   Look(TEXT("truck"));if(!Use(TEXT("truck")))return;Next(203,.2);return;
  }
  if(Stage==203)
  {
   if(M()->World->bDriving)
   {
    SawDriving=true;const double Distance=(M()->World->Odometer()-DriveOdo)*1609.344;
    if(Distance<0||M()->World->bCommuteVehicle){Check(false,TEXT("ordinary SARO→diner drive lost actual service truck/continuous odometer"));return;}
    if(LastDriveLeg!=M()->World->Leg)
    {
     LastDriveLeg=M()->World->Leg;auto R=S47J::New();S47J::Set(R,TEXT("name"),TEXT("actual-diner-drive-leg"));S47J::Set(R,TEXT("leg"),LastDriveLeg);S47J::Set(R,TEXT("area"),M()->World->Area);
     S47J::Set(R,TEXT("clock"),M()->Story->Clock());S47J::Set(R,TEXT("odometer"),M()->World->Odometer());S47J::Set(R,TEXT("actual_truck_three_m"),M()->World->CabTruck()?M()->World->CabTruck()->Position.ToString():TEXT("missing"));Transitions.Add(MakeShared<FJsonValueObject>(R));
    }
    return;
   }
   if(!SawDriving){if(Elapsed-Stalled>20)Result(false,TEXT("actual truck OnUse never began the production diner drive"));return;}
   Driven=(M()->World->Odometer()-DriveOdo)*1609.344;M()->World->Autopilot.Reset();
   auto R=S47J::New();S47J::Set(R,TEXT("name"),TEXT("ordinary-saro-to-diner"));S47J::Set(R,TEXT("driven_metres"),Driven);S47J::Set(R,TEXT("odometer_before"),DriveOdo);S47J::Set(R,TEXT("odometer_after"),M()->World->Odometer());S47J::Set(R,TEXT("autopilot_target_mps"),20);S47J::Set(R,TEXT("actual_state"),M()->Flow->GameState());Transitions.Add(MakeShared<FJsonValueObject>(R));
   if(!Check(Driven>1000&&M()->World->Area==TEXT("diner")&&M()->World->TruckAt==TEXT("diner")&&M()->World->TruckOnLot()&&C()->S.bArrived&&C()->S.Stage==TEXT("diner")
    &&M()->Story->Clock()<5*3600+15*60&&!C()->S.SC12.bDeputyHere&&!C()->ReadyForRoad(),TEXT("ordinary actual diner drive parks before05:15 sheriff, without clock hold/rewind; source comparison not yet performed"))||!SourceIntegrity(TEXT("actual diner arrival")))return;
   if(GoItem(TEXT("diner:booth")))Next(204,0);return;
  }
  if(Stage==204){if(Walk(Dt)){Look(TEXT("diner:booth"));Next(1,.35);}return;}
  if (Stage == 1)
  {
   Look(TEXT("diner:booth")); if (!Use(TEXT("diner:booth"))) return;
   C()->Choose(TEXT("compare")); if(!ActualInput)C()->Who = TEXT("ward"); C()->Choose(TEXT("ack-warning")); C()->Choose(TEXT("ack-route")); C()->Finish();
   if (!Check(!C()->SourcesVerified() && !C()->S.SC12.bWarningSent && !C()->S.SC12.bCourier && !M()->Story->Begun.Contains(TEXT("ch6")), ActualInput?TEXT("actual complete originals without required reading/checks: unoffered early compare/ack/departure grant no conclusion"):TEXT("one missing source and out-of-order compare/ack/departure produce no conclusion or progress"))) return;
   Close(); if (!Restore(ActualInput?TEXT("actual-sources-before-required-reading"):TEXT("incomplete-sources"))) return; Next(2); return;
  }
  if (Stage == 2)
  {
   if(!ActualInput)M()->Story->AddDoc(Third);
   for (int32 I = 0; I < 3; ++I) { C()->OpenSources(); if (!Choice(FString::Printf(TEXT("read%d"), I))) return; Close(); }
   C()->OpenSources(); if (!Choice(TEXT("origins")) || !Choice(TEXT("clocks")) || !Choice(TEXT("feeds"))) return;
   if (!Check(C()->S.SC12.ReadMask == 7 && C()->S.SC12.CheckMask == 7 && !C()->S.SC12.bCompared && !C()->S.SC12.bWarningSent, TEXT("reading and all three explicit checks still require the comparison action"))) return;
   Close(); if (!Restore(TEXT("source-checks-before-conclusion"))) return; Next(3); return;
  }
  if (Stage == 3)
  {
   C()->OpenSources(); if (!Choice(TEXT("compare"))) return; Close();
   if (!Check(C()->SourcesVerified() && !C()->S.SC12.bWarningSent && M()->Story->Clock() >= 5 * 3600 + 15 * 60 && M()->Story->Clock() < S47Timeline::Departure, TEXT("actual completed comparison advances to05:15 but sends no warning"))) return;
   if (!Restore(TEXT("sources-compared"))) return; Next(4); return;
  }
  if (Stage == 4)
  {
   const auto R = C()->SC12Readback();
   if (!Check(C()->S.SC12.bDeputyHere && !S47J::Bool(R, TEXT("deputyAtOldOffice")) && !S47J::Bool(R, TEXT("oldPatrolCarOut")) && !S47J::Bool(R, TEXT("oldSC03DeputyVisible")) && C()->SC12Object(TEXT("deputyBody")).Shown() && C()->SC12Object(TEXT("patrol")).Shown(), TEXT("05:15 actual diner deputy and patrol paper; retained SC03 body and old office/car are not simultaneously visible"))) return;
   WalkTo({FVector(-1.4,0,3.05), FVector(-1.4,0,7.1), FVector(-.8,0,7.25)}); Next(5,0); return;
  }
  if (Stage == 5) { if (!Walk(Dt)) return; Look(TEXT("diner:payphone")); Next(6,.5); return; }
  if (Stage == 6)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("send-warning"))) return;
   if (!Check(C()->PendingReadback == 1 && !C()->S.SC12.bWarningSent && !M()->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("HR_WARNING_RECEIPT"); })
    && C()->Lines.ContainsByPredicate([](const S47Panels::FTalkLine& L) { return L.Text.Contains(TEXT("Hondo local clock / 04:56:08")) && L.Text.Contains(TEXT("04:56:09")); }),
    TEXT("three IDs, Hondo04:56:08 and separate SARO04:56:09 displayed in readback, not yet acknowledged"))) return;
   Close(); if (!Check(C()->PendingReadback == 0 && !C()->S.SC12.bWarningSent, TEXT("hanging up before confirmation gives no receipt")) || !Restore(TEXT("interrupted-warning"))) return;
   Next(7,.5); return;
  }
  if (Stage == 7)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("send-warning")) || !Choice(TEXT("ack-warning"))) return;
   if (!Check(C()->S.SC12.bWarningSent && !C()->S.SC12.WarningStamp.IsEmpty() && !C()->S.SC12.bCourier && C()->S.Stage == TEXT("diner"), TEXT("explicit readback confirmation writes dated warning; no courier or road credit"))) return;
   Close(); if (!Restore(TEXT("warning-receipted"))) return;
   WalkTo({FVector(-2,0,7.25), FVector(-2,0,1.25), FVector(-2.8,0,1.25)}); Next(8,0); return;
  }
  if (Stage == 8) { if (!Walk(Dt)) return; Look(TEXT("sc12:patrol")); Next(9,.5); return; }
  if (Stage == 9)
  {
   Look(TEXT("sc12:patrol")); if (!Use(TEXT("sc12:patrol"))) return;
   if (!Check(C()->S.SC12.bPatrolRead && C()->PatrolDoc().Page.Contains(TEXT("04:15 MST")) && C()->PatrolDoc().Page.Contains(TEXT("05:15")) && !C()->S.SC12.bCourier, TEXT("actual dated counter paper read, one04:15 drive and05:15 arrival, no guaranteed safe road or courier credit"))) return;
   Close(); if (!Restore(TEXT("patrol-read"))) return;
   WalkTo({FVector(-2,0,1.25), FVector(-2,0,3.05), FVector(-.8,0,3.05)}); Next(10,0); return;
  }
  if (Stage == 10) { if (!Walk(Dt)) return; Look(TEXT("diner:booth")); Next(11,.5); return; }
  if (Stage == 11)
  {
   Look(TEXT("diner:booth")); if (!Use(TEXT("diner:booth")) || !Choice(TEXT("map"))) return;
   C()->MoveTracing(300,690); const auto Early = C()->Answer(TEXT("bm8"));
   if (!Check(!Early.bOk && !C()->S.bP13 && C()->S.Wrong13 == 0, TEXT("correct guessed crossing before actual tracing alignment records nothing"))) return;
   const auto Station = S47Drawings::SERVICE_MAP().Station; C()->MoveTracing(Station.X,Station.Y);
   if (!Check(!C()->Answer(TEXT("bm6")).bOk && C()->S.Wrong13 == 1 && !C()->S.bP13, TEXT("aligned wrong bolt remains wrong, counted independently of warning"))) return;
   Close(); if (!Restore(TEXT("aligned-map-wrong-answer"))) return; Next(12); return;
  }
  if (Stage == 12)
  {
   if (!Check(C()->Aligned() && C()->S.Wrong13 == 1 && !C()->S.bP13, TEXT("aligned tracing and wrong count restore before correct answer"))) return;
   C()->OpenMap(); if (!Check(C()->Answer(TEXT("bm8")).bOk && C()->S.bP13 && C()->S.Stage == TEXT("diner") && C()->DocFinding().Page.Contains(TEXT("8.15")) && C()->DocFinding().Page.Contains(TEXT("10.4")) && !C()->TruckTrip(TEXT("diner")), TEXT("P13 separates C8.15 and proposed courier10.4, but cannot release old road without Ward's route readback"))) return;
   Close(); if (!Restore(TEXT("p13-before-courier"))) return;
   WalkTo({FVector(-1.4,0,3.05), FVector(-1.4,0,7.1), FVector(-.8,0,7.25)}); Next(13,0); return;
  }
  if (Stage == 13) { if (!Walk(Dt)) return; Look(TEXT("diner:payphone")); Next(14,.5); return; }
  if (Stage == 14)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone"))) return;
   C()->Choose(TEXT("ack-route")); if (!Check(!C()->S.SC12.bCourier && !C()->ReadyForRoad(), TEXT("unsolicited route acknowledgement does not confirm a courier")) || !Choice(TEXT("request-courier"))) return;
   if (!Check(C()->PendingReadback == 2 && !C()->S.SC12.bCourier && C()->Lines.ContainsByPredicate([](const S47Panels::FTalkLine& L) { return L.Text.Contains(TEXT("not a map of everywhere")); }), TEXT("Ward's actual route readback includes patrol limits and a choice to remain in refuge"))) return;
   Close(); if (!Restore(TEXT("interrupted-courier-readback"))) return; Next(15,.5); return;
  }
  if (Stage == 15)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("request-courier")) || !Choice(TEXT("ack-route"))) return;
   if (!Check(C()->ReadyForRoad() && C()->S.Stage == TEXT("road") && C()->TruckTrip(TEXT("diner")).IsSet() && !M()->Story->Begun.Contains(TEXT("ch6")), TEXT("only complete warning/patrol/P13/courier readback releases departure choice"))) return;
   Close(); if (!Restore(TEXT("complete-route-before-departure"))) return; Next(16); return;
  }
  if (Stage == 16)
  {
   if (!Check(C()->ReadyForRoad() && RayUses >= 6 && Restores >= 8 && WalkMetres > 12, TEXT("complete source/readback/action-order/disk journey and physical diner legs executed"))) return;
   const auto Trip = C()->TruckTrip(TEXT("diner")); if (!Check(Trip && bool(Trip->Use), TEXT("production service-truck departure choice exists"))) return;
   Trip->Use();
   if (!Check(FMath::Abs(M()->Story->Clock() - S47Timeline::Departure) < .001 && C()->S.Stage == TEXT("road"), TEXT("normal chosen departure reaches05:20 while stable saved checkpoint still says road"))) return;
   if(ActualInput)
   {
    if(!ExportBoundary()||!Snapshot(TEXT("actual-safe-departure-after-continue")))return;
    Result(true,TEXT("SC12 actual SC11 sources, ordinary SARO→diner travel, independent comparison/Ward/patrol/P13/courier and safe full disk/title departure"));return;
   }
   M()->Flow->FlushAutosave(); const auto Meta = M()->Flow->Saves.Latest(1); FS47SaveStore Reader(M()->Flow->Saves.Dir); Reader.Init(); const auto Disk = Meta ? Reader.Load(Meta->Id) : FJ();
   const auto Ch5 = S47J::Obj(S47J::Obj(Disk,TEXT("case")),TEXT("ch5"));
   if (!Check(Disk && S47J::Str(Disk,TEXT("checkpoint")) == TEXT("chapter5") && S47J::Str(Ch5,TEXT("stage")) == TEXT("road") && S47J::Bool(S47J::Obj(Ch5,TEXT("sc12")),TEXT("routeReadback")) && S47J::Num(S47J::Obj(Disk,TEXT("case")),TEXT("clock")) == S47Timeline::Departure, TEXT("fresh disk reader sees ordinary safe05:20 departure checkpoint before chapter6"))) return;
   if (!Write(TEXT("departure-checkpoint.disk.json"), Disk)) { Result(false,TEXT("departure disk evidence write failed")); return; }
   TArray<uint8> Bytes;if(!Check(FFileHelper::LoadFileToArray(Bytes,*(Root/TEXT("departure-checkpoint.disk.json")))&&FFileHelper::SaveArrayToFile(Bytes,*(Root/TEXT("sc12-complete.disk.json")))
    &&SC12Hash(Root/TEXT("departure-checkpoint.disk.json"))==SC12Hash(Root/TEXT("sc12-complete.disk.json")),TEXT("named legacy fixture also retains byteidentical common output; empty chain remains ineligible as actual predecessor")))return;
   CheckpointFile=SC12File(Root/TEXT("sc12-complete.disk.json"));DepartureFile=SC12File(Root/TEXT("departure-checkpoint.disk.json"));
   Next(17,1); return;
  }
  if (Stage == 17)
  {
   if (!Check(M()->Story->Begun.Contains(TEXT("ch6")) && !C()->bActive && C()->S.Stage == TEXT("complete") && M()->Story->Clock() >= S47Timeline::Departure && M()->Story->Clock() < S47Timeline::Finale, TEXT("normal chapter6 continuation begins only after complete SC12; finale time remains reserved"))) return;
   if (!Snapshot(TEXT("departed-to-sc13"))) return;
   Result(true,TEXT("SC12 source comparison, text-only Ward receipts, physical dated patrol sheet, P13, chosen courier route and stable full Flow restores"));
  }
 }
};

void US47Chapter5::ControlSC12Test(double Dt)
{
 if (!SC12Contract) SC12Contract = MakeShared<FS47SC12Contract>(M.Get());
 const auto KeepAlive = SC12Contract; // Full Flow resets this chapter's state, not the in-flight test's lifetime.
 KeepAlive->Tick(Dt);
}
