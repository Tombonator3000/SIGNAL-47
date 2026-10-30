#include "S47NightContract.h"
#include "S47Album.h"
#include "S47DishArray.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47HorrorTest.h"
#include "S47HorrorThreat.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Observing.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47ThreatNavigation.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47ScreenView.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
 using FJ=S47J::FObj;
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 FString Full(FString P){P=FPaths::ConvertRelativePathToFull(P);FPaths::CollapseRelativeDirectories(P);FPaths::NormalizeFilename(P);return P;}
 FString Sha(const FString& P)
 {
  TArray<uint8>B;if(!FFileHelper::LoadFileToArray(B,*P))return {};
  FSHAHash H;FSHA1::HashBuffer(B.GetData(),B.Num(),H.Hash);return H.ToString();
 }
 FJ File(const FString& P)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("path"),P);S47J::Set(O,TEXT("bytes"),double(IFileManager::Get().FileSize(*P)));S47J::Set(O,TEXT("sha1"),Sha(P));return O;
 }
 bool Write(const FString& P,const FJ& O){return O&&FFileHelper::SaveStringToFile(S47J::Write(O,true),*P);}
 FJ Encoders(US47Main* M)
 {
  auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> A;
  for(int32 I=0;I<M->Web->DishCount();++I)A.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(M->Web->DishAz(I)),MakeShared<FJsonValueNumber>(M->Web->DishEl(I))}));
  O->SetArrayField(TEXT("pointing"),A);return O;
 }
 FJ Papers(US47Main* M)
 {
  auto O=S47J::New();O->SetArrayField(TEXT("docs"),S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")));return O;
 }
}

bool S47NightContract::RoundTrip(US47Main* M,const FString& Root,const FString& Name,FJ& Evidence,FString& Error,bool Restore)
{
 const auto Bad=[&](const FString& Why){Error=Name+TEXT(": ")+Why;return false;};
 if(!M||!M->Flow||!M->Story||!M->World||!M->Player||!M->Web||M->Flow->CaseId!=1||M->Flow->bDevRun)return Bad(TEXT("isolated real case1 required"));
 const FString Block=M->Flow->SaveBlock();
 if(!Block.IsEmpty()||M->World->bDriving||M->World->bBusy||M->Flow->bStarting)return Bad(TEXT("ordinary save blocked; never force a checkpoint: ")+Block);
 const FJ Before=M->Flow->GameState(),Docs=Papers(M),Pointing=Encoders(M);const double Clock=M->Story->Clock();
 const FVector Pose=M->Player->Pos;const double Yaw=M->Player->Yaw,Pitch=M->Player->Pitch;
 if(!Before||M->Web->DishCount()!=27||!M->Player->Walkable(Pose.X,Pose.Z))return Bad(TEXT("actual complete state/27encoders/walkable pose missing"));
 if(!IFileManager::Get().MakeDirectory(*Root,true)||IFileManager::Get().FileExists(*(Root/(Name+TEXT(".disk.json")))))return Bad(TEXT("fresh own checkpoint output required"));
 if(!Write(Root/(Name+TEXT(".before.json")),Before))return Bad(TEXT("cannot retain actual raw state before manual save"));
 const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();
 const FJ Disk=Meta?Reader.Load(Meta->Id):FJ();
 if(!Meta||!Same(Before,Disk))return Bad(TEXT("ordinary manual/fresh disk reader differs: ")+Error);
 const FString DiskPath=Root/(Name+TEXT(".disk.json"));if(!Write(DiskPath,Disk))return Bad(TEXT("complete disk evidence write failed"));
 Evidence=S47J::New();S47J::Set(Evidence,TEXT("name"),Name);S47J::Set(Evidence,TEXT("checkpoint_file"),File(DiskPath));
 S47J::Set(Evidence,TEXT("clock_before"),Clock);S47J::Set(Evidence,TEXT("documents_before"),Docs);S47J::Set(Evidence,TEXT("encoders_before"),Pointing);
 S47J::Set(Evidence,TEXT("ordinary_save_block"),Block);S47J::Set(Evidence,TEXT("restored"),Restore);
 if(!Restore){S47J::Set(Evidence,TEXT("pass"),true);return Write(Root/(Name+TEXT(".roundtrip.json")),Evidence)||Bad(TEXT("checkpoint receipt write failed"));}
 M->Player->TestMove.Reset();M->Flow->ToTitle();
 FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),Live=M->Flow->Saves.ContinueSave();
 const FJ SelectedDisk=Selected?TitleReader.Load(Selected->Id):FJ();
 if(M->Mode!=US47Main::EMode::Title||!Selected||!Live||Selected->Id!=Live->Id||!Same(SelectedDisk,Disk))return Bad(TEXT("ordinary title/fresh Continue selection differs"));
 if(!M->Flow->OpenTestView(TEXT("continue")))return Bad(TEXT("ordinary title Continue route unavailable"));
 const auto Actual=M->Flow->GameState();bool Good=M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->Story->Clock()==Clock&&
  M->World->Area==S47J::Str(Before,TEXT("area"))&&M->Flow->CheckpointName()==S47J::Str(Before,TEXT("checkpoint"))&&
  FMath::Abs(M->Player->Pos.X-Pose.X)<.0001&&FMath::Abs(M->Player->Pos.Z-Pose.Z)<.0001&&FMath::Abs(M->Player->Yaw-Yaw)<.00001&&FMath::Abs(M->Player->Pitch-Pitch)<.00001&&
  Same(Papers(M),Docs)&&Same(Encoders(M),Pointing);
 auto Stable=S47J::New();
 for(const TCHAR* Key:{TEXT("horror"),TEXT("observing"),TEXT("substation"),TEXT("doors")})
 {
  const auto A=Before->TryGetField(Key),B=Actual->TryGetField(Key);
  const bool Match=(!A&&!B)||(A&&B&&FJsonValue::CompareEqual(*A,*B));S47J::Set(Stable,Key,Match);Good&=Match;
 }
 S47J::Set(Evidence,TEXT("stable_production_fields"),Stable);S47J::Set(Evidence,TEXT("clock_after"),M->Story->Clock());
 S47J::Set(Evidence,TEXT("documents_after"),Papers(M));S47J::Set(Evidence,TEXT("encoders_after"),Encoders(M));S47J::Set(Evidence,TEXT("pass"),Good);
 if(!Write(Root/(Name+TEXT(".actual.json")),Actual)||!Write(Root/(Name+TEXT(".roundtrip.json")),Evidence))return Bad(TEXT("raw actual/roundtrip evidence write failed"));
 return Good||Bad(TEXT("full Flow changed clock/pose/sourcebytes/27encoders or protected production state; raw before/actual retained"));
}

class FS47NightContract : public TSharedFromThis<FS47NightContract>
{
 TWeakObjectPtr<US47Main> Owner;
 US47Main* M()const{return Owner.Get();}
 FString Root,Chain,D01Line,LastLeg;
 int32 Stage=0,Checks=0,Cursor=0,WalkSamples=0,RayUses=0,Restores=0;
 double Began=FPlatformTime::Seconds(),Mark=Began,Wait=0,LineAt=0,Walked=0,Driven=0,LastClock=-1e9;
 bool Started=false,Done=false;
 FVector LastFoot=FVector::ZeroVector,LastDrive=FVector::ZeroVector,AimPoint=FVector::ZeroVector;
 TArray<FVector> Route;
 TArray<TSharedPtr<FJsonValue>> Evidence,States,Transitions,Actions;
 TArray<FLinearColor> BeaconBaseline;
 FJ HeldPapers,PreviousBoundary,PreviousResult;
 void Next(int32 N,double Hold=.2){Stage=N;Mark=FPlatformTime::Seconds();Wait=Hold;}
 bool Record(const FString& Name)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("game_state"),M()->Flow->GameState());S47J::Set(O,TEXT("clock"),M()->Story->Clock());
  S47J::Set(O,TEXT("adapter"),M()->HorrorTest->NightReadback());S47J::Set(O,TEXT("pointing"),Encoders(M()));
  const auto F=M()->Substation->FaultSnapshot();auto Fault=S47J::New();
  S47J::Set(Fault,TEXT("requested"),F.bRequested);S47J::Set(Fault,TEXT("active"),F.bActive);S47J::Set(Fault,TEXT("arc_visible"),F.bArcVisible);S47J::Set(Fault,TEXT("lamps_visible"),F.bLampsVisible);
  S47J::Set(Fault,TEXT("arc_lumens"),F.ArcLumens);S47J::Set(Fault,TEXT("lamp_lumens"),F.LampLumens);S47J::Set(Fault,TEXT("strike_bursts"),M()->Substation->ArcCount());S47J::Set(O,TEXT("fault"),Fault);
  States.Add(MakeShared<FJsonValueObject>(O));return Write(Root/(Name+TEXT(".raw.json")),O);
 }
 void Finish(bool Good,const FString& Why)
 {
  const auto Keep=AsShared();if(Done)return;Done=true;M()->Player->TestMove.Reset();M()->Player->bTestRun=false;
  auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS_PREFIX_ONLY"):TEXT("FAIL"));S47J::Set(O,TEXT("scenario"),TEXT("NIGHT_SC01_SC03_PREFIX"));
  S47J::Set(O,TEXT("complete_night_accepted"),false);S47J::Set(O,TEXT("chain_id"),Chain);S47J::Set(O,TEXT("process_id"),double(FPlatformProcess::GetCurrentProcessId()));
  S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("restores"),Restores);S47J::Set(O,TEXT("walked_metres"),Walked);
  S47J::Set(O,TEXT("driven_metres"),Driven);S47J::Set(O,TEXT("walk_samples"),WalkSamples);S47J::Set(O,TEXT("actual_ray_uses"),RayUses);
  O->SetArrayField(TEXT("executed_checks"),Evidence);O->SetArrayField(TEXT("states"),States);O->SetArrayField(TEXT("transitions"),Transitions);O->SetArrayField(TEXT("actions"),Actions);
  S47J::Set(O,TEXT("actual_horror_route"),M()->HorrorTest->NightReadback());
  S47J::Set(O,TEXT("sc03_complete_checkpoint"),Root/TEXT("sc03/sc03-complete.disk.json"));
  if(IFileManager::Get().FileExists(*(Root/TEXT("sc03/sc03-complete.disk.json"))))S47J::Set(O,TEXT("checkpoint_file"),File(Root/TEXT("sc03/sc03-complete.disk.json")));
  S47J::Set(O,TEXT("limits"),TEXT("Actual normal newcase1/commute/park/D01 and continuous SC01-observing/outage/T01/SC02/SC03 production cohort. Driving uses explicitly labelled native autopilot/route-target APIs; receiver dials use production APIs. Legacy test-only budget/revision/prerequisite fixtures are excluded. Product capture/retry and normal drive/park/load placement remain product behavior, never a test recovery. SC04-SC13, full-night pacing, physical input/listening/visual/performance/package/install acceptance remain UNVERIFIED."));
  S47J::Set(O,TEXT("human_input"),false);S47J::Set(O,TEXT("human_visual_acceptance"),false);S47J::Set(O,TEXT("installed"),false);
  const bool Saved=!Root.IsEmpty()&&Write(Root/TEXT("result.json"),O);
  if(Good&&Saved){UE_LOG(LogTemp,Display,TEXT("S47: night contract SC01-SC03 PREFIX PASS COMPLETE: %d checks, evidence %s; SC04-SC13 UNVERIFIED"),Checks,*Root);}
  else{UE_LOG(LogTemp,Error,TEXT("S47: night contract FAIL stage %d: %s evidence=%s"),Stage,*Why,*Root);}
  FPlatformMisc::RequestExitWithStatus(false,Good&&Saved?0:1);
 }
 bool Check(bool Good,const FString& Why)
 {
  ++Checks;auto R=S47J::New();S47J::Set(R,TEXT("check"),Checks);S47J::Set(R,TEXT("stage"),Stage);S47J::Set(R,TEXT("pass"),Good);S47J::Set(R,TEXT("detail"),Why);Evidence.Add(MakeShared<FJsonValueObject>(R));
  if(!Good){Finish(false,Why);return false;}return true;
 }
 FS47Obj Object(const FString& Id)const
 {
  const auto* I=M()->Inter->Get(Id);if(I&&I->Object)return I->Object;const auto* E=M()->Web->Item(Id);return E?M()->Web->ObjAt(E->Node):FS47Obj();
 }
 bool Face(const FString& Id)
 {
  const auto O=Object(Id);const auto B=O.WorldBox();if(!Check(O&&B.IsValid,TEXT("actual retained interaction target: ")+Id))return false;
  const FVector At=B.GetCenter(),D=At-S47Three::ToThree(M()->Player->EyeUE());AimPoint=S47Three::ToUE(At);
  M()->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M()->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
 }
 bool Use(const FString& Id,bool Activate=true)
 {
  const FVector E=M()->Player->EyeUE(),D=M()->Player->ForwardUE();const FString Pick=M()->Inter->Pick(E,D),Focus=M()->Inter->Update(E,D);
  auto O=S47J::New();S47J::Set(O,TEXT("item"),Id);S47J::Set(O,TEXT("picked"),Pick);S47J::Set(O,TEXT("focus"),Focus);S47J::Set(O,TEXT("eye_ue_cm"),E.ToString());S47J::Set(O,TEXT("forward_ue"),D.ToString());
  S47J::Set(O,TEXT("activate_bound_hud_onuse"),Activate);Actions.Add(MakeShared<FJsonValueObject>(O));
  if(!Check(!M()->ModalOpen()&&Pick==Id&&Focus==Id&&M()->Player->FocusedId()==Id&&FVector::DotProduct(D,(AimPoint-E).GetSafeNormal())>.995&&(!Activate||bool(M()->Player->OnUse)),TEXT("actual settled camera Pick/Update/focus and ordinary HUD use: ")+Id))return false;
  if(Activate){M()->Player->OnUse(Pick);++RayUses;}return true;
 }
 void Path(TArray<FVector> P)
 {
  M()->Player->TestMove.Reset();Route=MoveTemp(P);Cursor=0;LastFoot=M()->Player->Pos;Mark=FPlatformTime::Seconds();
 }
 bool Walk()
 {
  if(M()->ModalOpen()||M()->Story->IsPaused()){M()->Player->TestMove.Reset();LastFoot=M()->Player->Pos;return false;}
  auto* P=M()->Player.Get();const double Dist=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();
  if(!Check(Dist<.5&&P->Walkable(P->Pos.X,P->Pos.Z),TEXT("night actual native Walker/Resolve remains continuous/walkable; no Place recovery")))return false;
  Walked+=Dist;++WalkSamples;LastFoot=P->Pos;
  if(!Route.IsValidIndex(Cursor)){P->TestMove.Reset();return true;}
  const FVector D=Route[Cursor]-P->Pos;const double Length=FVector2D(D.X,D.Z).Size();
  if(Length<.07){++Cursor;P->TestMove.Reset();Mark=FPlatformTime::Seconds();return false;}
  if(FPlatformTime::Seconds()-Mark>35){Finish(false,FString::Printf(TEXT("native night route blocked at %.3f/%.3f to %.3f/%.3f"),P->Pos.X,P->Pos.Z,Route[Cursor].X,Route[Cursor].Z));return false;}
  P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=-.06;P->TestMove=FVector2D(0,-FMath::Min(1.,Length*2));P->bTestRun=false;return false;
 }
 bool GoItem(const FString& Id)
 {
  const auto O=Object(Id);const auto B=O.WorldBox();if(!Check(O&&B.IsValid,TEXT("actual return-target bounds: ")+Id))return false;
  const auto* Item=M()->Inter->Get(Id);const double Range=Item&&Item->Range>0?Item->Range:3.2;
  const FVector C=B.GetCenter();const FVector2D From(M()->Player->Pos.X,M()->Player->Pos.Z);
  TArray<FVector2D> Seeds;const double Radius=FMath::Min(100.,FVector2D(C.X-From.X,C.Z-From.Y).Size()+10);
  for(int32 I=0;I<16;++I){const double A=I*UE_DOUBLE_PI/8;Seeds.Add(From+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius*.5);}
  TArray<FVector2D> Best;double BestLength=1e100;
  for(double R:{1.5,2.,2.7})for(int32 I=0;I<16;++I)
  {
   const double A=I*UE_DOUBLE_PI/8;const FVector2D To(C.X+R*FMath::Cos(A),C.Z+R*FMath::Sin(A));
   if(!M()->Player->Walkable(To.X,To.Y))continue;const FVector Eye(To.X,M()->Player->FloorY+M()->Player->Eye,To.Y),D=C-Eye;
   if(D.Size()>Range-.08||M()->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)continue;
   auto Found=FS47ThreatNavigation::FindPath(From,To,Seeds,M()->Web->Colliders(),M()->Player->Radius,[&](const FVector2D& X,const FVector2D& Y)
   {const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(X,Y)/.05));for(int32 J=0;J<=N;++J){const auto Q=FMath::Lerp(X,Y,double(J)/N);if(!M()->Player->Walkable(Q.X,Q.Y))return false;}return true;});
   double Length=0;auto Previous=From;for(const auto& P:Found){Length+=FVector2D::Distance(Previous,P);Previous=P;}
   if(!Found.IsEmpty()&&Length<BestLength){Best=MoveTemp(Found);BestLength=Length;}
  }
  if(!Check(!Best.IsEmpty(),TEXT("bounded measured walkable route reaches native ray target; no fallback: ")+Id))return false;
  TArray<FVector> P;for(const auto& V:Best)P.Add(FVector(V.X,0,V.Y));Path(MoveTemp(P));return true;
 }
 bool Cycle(const FString& Name)
 {
  FJ Proof;FString Error;if(!S47NightContract::RoundTrip(M(),Root,Name,Proof,Error)){Finish(false,Error);return false;}
  ++Restores;Transitions.Add(MakeShared<FJsonValueObject>(Proof));return true;
 }
 bool Boundary(const FString& Id)
 {
  if(HeldPapers)
  {
   const auto Actual=S47J::Arr(Papers(M()),TEXT("docs"));
   for(const auto& Before:S47J::Arr(HeldPapers,TEXT("docs")))
   {
    const FString DocId=S47J::Str(Before->AsObject(),TEXT("id"));
    const auto* After=Actual.FindByPredicate([&](const TSharedPtr<FJsonValue>& V){return V&&V->Type==EJson::Object&&S47J::Str(V->AsObject(),TEXT("id"))==DocId;});
    if(!Check(After&&FJsonValue::CompareEqual(*Before,**After),TEXT("actual SC01 source retains all fields/sheets/stamp across field trip and later actions: ")+DocId))return false;
   }
  }
  const FString Folder=Root/Id;FJ Proof;FString Error;
  if(!S47NightContract::RoundTrip(M(),Folder,Id+TEXT("-complete"),Proof,Error)){Finish(false,Error);return false;}++Restores;
  auto R=S47J::New();S47J::Set(R,TEXT("status"),TEXT("PASS"));S47J::Set(R,TEXT("scenario"),Id.ToUpper());S47J::Set(R,TEXT("mode"),TEXT("night"));
  S47J::Set(R,TEXT("chain_id"),Chain);S47J::Set(R,TEXT("process_id"),double(FPlatformProcess::GetCurrentProcessId()));S47J::Set(R,TEXT("checkpoint_file"),S47J::Obj(Proof,TEXT("checkpoint_file")));
  S47J::Set(R,TEXT("previous_checkpoint_file"),PreviousBoundary);S47J::Set(R,TEXT("previous_result_file"),PreviousResult);
  S47J::Set(R,TEXT("ordinary_new_night_source"),File(Root/TEXT("ordinary-new-night.raw.json")));
  S47J::Set(R,*(Id+TEXT("_complete_checkpoint")),Folder/(Id+TEXT("-complete.disk.json")));S47J::Set(R,TEXT("normal_title_continue"),true);
  S47J::Set(R,TEXT("source_prerequisite_fixtures"),false);S47J::Set(R,TEXT("actual_horror_route"),M()->HorrorTest->NightReadback());S47J::Set(R,TEXT("roundtrip"),Proof);
  R->SetArrayField(TEXT("executed_checks"),Evidence);R->SetArrayField(TEXT("transitions"),Transitions);
  S47J::Set(R,TEXT("limits"),TEXT("New actual night cohort and full real save state; only this scenario/prefix is accepted. Later scenarios remain unexecuted. Autopilot/receiver/conversation-control APIs are not physical human input or subjective listening/visual approval."));
  if(!Check(Write(Folder/TEXT("result.json"),R),TEXT("complete genuine production checkpoint and sibling scenario PASS: ")+Id))return false;
  PreviousBoundary=S47J::Obj(Proof,TEXT("checkpoint_file"));PreviousResult=File(Folder/TEXT("result.json"));return true;
 }
 bool Power(bool Out)
 {
  if(!Check(M()->SaroNight&&M()->SaroNight->Dishes&&BeaconBaseline.Num()==27&&M()->SaroNight->Dishes->BeaconCount()==27,TEXT("actual27 beacon baseline retained")))return false;
  auto R=S47J::New();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;const double Factor=Out?.12:1;
  for(int32 I=0;I<27;++I)
  {
   const auto Actual=M()->SaroNight->Dishes->BeaconBase(I),Base=BeaconBaseline[I];const FLinearColor Expected(Base.R*Factor,Base.G*Factor,Base.B*Factor,1);
   const bool Match=Actual.Equals(Expected,.00001f);Good&=Match;auto Row=S47J::New();S47J::Set(Row,TEXT("index"),I);S47J::Set(Row,TEXT("actual"),TArray<double>{Actual.R,Actual.G,Actual.B,Actual.A});
   S47J::Set(Row,TEXT("baseline"),TArray<double>{Base.R,Base.G,Base.B,Base.A});S47J::Set(Row,TEXT("match"),Match);Rows.Add(MakeShared<FJsonValueObject>(Row));
  }
  S47J::Set(R,TEXT("factor"),Factor);R->SetArrayField(TEXT("actual_beacons"),Rows);States.Add(MakeShared<FJsonValueObject>(R));
  return Check(Good,Out?TEXT("actual feeder outage uses12percent of original27 beacon colors"):TEXT("actual T01 restores original27 beacon colors after Continues"));
 }
 bool Begin();
public:
 explicit FS47NightContract(US47Main* Main):Owner(Main){}
 void Tick(double Dt);
};

bool FS47NightContract::Begin()
{
 FString Mode,Profile;FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Mode);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Profile);Profile=Full(Profile);
 const FString Saved=Full(FPaths::ProjectSavedDir()),Store=Full(M()->Flow->Saves.Dir);
 if(!Check(Mode==TEXT("night")&&Profile.StartsWith(TEXT("/tmp/"))&&Profile!=TEXT("/tmp")&&FPaths::IsUnderDirectory(Saved,Profile)&&FPaths::IsUnderDirectory(Store,Profile)&&
  M()->Flow->CaseId==0&&M()->Flow->bDevRun&&M()->Flow->Saves.List().IsEmpty()&&!IFileManager::Get().DirectoryExists(*(Saved/TEXT("NightContract"))),TEXT("fresh explicit/tmp Saved/Saves containment before first proof or case write")))return false;
 for(const TCHAR* Flag:{TEXT("S47Jump"),TEXT("S47HorrorContract"),TEXT("S47NightSequenceContract"),TEXT("S47NightSequenceSC01"),TEXT("S47NightSequenceEscape"),TEXT("S47EveningRoute"),TEXT("S47HorrorArrival"),TEXT("S47Drive"),TEXT("S47GetOut"),TEXT("S47Auto"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47Face"),TEXT("S47WalkFrom"),TEXT("S47Shot"),TEXT("S47ProductionViews"),TEXT("S47T01Contract"),TEXT("S47T01DriveContract"),TEXT("S47PropsOff"),TEXT("S47FxOff")})
 {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value))return Check(false,FString(TEXT("night excludes independent driver/jump/prerequisite fixture: "))+Flag);}
 double Seconds=0;FParse::Value(FCommandLine::Get(),TEXT("S47Seconds="),Seconds);
 if(!Check(!FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest"))||(FMath::IsFinite(Seconds)&&Seconds>=5400),TEXT("optional focus SmokeTest needs5400s outer timer; outer exit never grants night PASS")))return false;
 Chain=FGuid::NewGuid().ToString(EGuidFormats::Digits);Root=Saved/TEXT("NightContract")/Chain;
 if(!IFileManager::Get().MakeDirectory(*Root,true)){Finish(false,TEXT("own fresh evidence directory unavailable"));return false;}
 Started=true;US47Flow::FStart NewNight;NewNight.CaseId=1;M()->Flow->StartGame(NewNight);
 if(!Check(!M()->Flow->bDevRun&&M()->Flow->CaseId==1&&M()->Evening->Commute()&&M()->World->bDriving&&M()->World->bCommuteVehicle&&M()->Horror->Enabled()&&M()->Spine->Enabled(),TEXT("ordinary empty-state/noJump newcase creates actual personal commute and fresh production cohort")))return false;
 if(!Check(M()->World->CabTruck()!=nullptr,TEXT("actual normal new-night cab exists before autopilot control")))return false;
 M()->World->Autopilot=16;LastDrive=M()->World->CabTruck()->Position;LastLeg=M()->World->Leg;Next(1,.5);
 if(!Record(TEXT("ordinary-new-night"))){Finish(false,TEXT("cannot write actual ordinary-new-night source record"));return false;}return true;
}

void FS47NightContract::Tick(double Dt)
{
 const auto Keep=AsShared();if(Done||!M()||!M()->Story||!M()->Flow||!M()->Player||!M()->World||!M()->Web||!M()->UI||!M()->Hud||!M()->Inter||!M()->Horror||!M()->HorrorTest||!M()->Evening||!M()->Observing||!M()->Substation||!M()->Spine||!M()->Night||!M()->Night->Album)return;
 if(!Started){if(!S47Startup::Blocking()&&!M()->Flow->bStarting&&M()->Mode==US47Main::EMode::Play)Begin();return;}
 const double Now=FPlatformTime::Seconds();if(Now-Began>4800){Finish(false,TEXT("4800s real night-prefix timeout; no fixture/recovery"));return;}
 const auto Adapter=M()->HorrorTest->NightReadback();if(S47J::Bool(Adapter,TEXT("failed"))){Finish(false,TEXT("actual HorrorTest night adapter rejected; raw component FAIL retained"));return;}
 if(M()->bPausedByMenu){Finish(false,TEXT("unexpected external activation pause; use explicit focus wrapper or focused window, never auto-resume"));return;}
 if(M()->Story->Clock()+.0001<LastClock){Finish(false,TEXT("night production clock moved backwards"));return;}LastClock=M()->Story->Clock();
 if(M()->World->bDriving&&M()->World->CabTruck())
 {
  const auto At=M()->World->CabTruck()->Position;const double D=FVector2D(At.X-LastDrive.X,At.Z-LastDrive.Z).Size();
  if(M()->World->Leg==LastLeg&&D<20)Driven+=D;LastDrive=At;LastLeg=M()->World->Leg;
 }
 if(S47Startup::Blocking()||M()->Flow->bStarting||M()->World->bBusy)return;if(Wait>0){Wait-=Dt;return;}
 switch(Stage)
 {
 case 1:
  if(M()->Evening->Commute()||M()->World->bDriving){if(Now-Mark>480)Finish(false,TEXT("ordinary personal commute did not physically park at SARO"));return;}
  if(!Check(M()->Evening->Opening()&&M()->Evening->Pack.Flag(TEXT("intro_arrived"))&&M()->World->Area==TEXT("saro")&&!M()->World->bCommuteVehicle&&Driven>1000,TEXT("actual leg-to-leg commute and normal visitor parking produce arrival; no handover credit yet"))||!Cycle(TEXT("arrival-before-d01")))return;
  Path({FVector(-15.2,0,18.3),FVector(-8.3,0,18.3),FVector(-8.3,0,12.2),FVector(-6.1,0,13.5),FVector(2,0,14.3),FVector(6,0,18),FVector(11.5,0,18),FVector(11.5,0,12),FVector(11.5,0,4),FVector(9.8,0,4),FVector(9.8,0,1.6),FVector(7.5,0,1.6)});Next(2,0);return;
 case 2:if(Walk()&&Face(TEXT("doorEast")))Next(3,.3);return;
 case 3:
  if(!M()->Web->IsDoorOpen(TEXT("doorEast"))&&!Use(TEXT("doorEast")))return;
  Path({FVector(5.2,0,1.6),FVector(4.2,0,1.6)});Next(4,0);return;
 case 4:if(Walk()&&Face(TEXT("phone")))Next(5,.3);return;
 case 5:
  if(!Use(TEXT("phone"))||!Check(M()->Evening->Pack.CurrentScene()&&M()->Evening->Pack.CurrentScene()->Id==TEXT("D01_HANDOVER"),TEXT("actual D01 starts through physical phone/HUD action")))return;
  Next(6);return;
 case 6:
 {
  auto& E=*M()->Evening;
  if(const auto* Line=E.Pack.CurrentLine())
  {
   if(D01Line!=Line->Id){D01Line=Line->Id;LineAt=Now;return;}if(Now-LineAt<FMath::Max(.7,Line->Duration+.5))return;
   if(!Check(M()->ModalOpen()&&FSlateApplication::IsInitialized(),TEXT("actual D01 displayed primary Continue/Slate available")))return;
   const bool Handled=FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
   if(!Check(Handled,TEXT("actual D01 focused Slate Enter after source line duration; no listening claim")))return;D01Line.Reset();return;
  }
  if(const auto* Scene=E.Pack.CurrentScene())
  {
   const FString Reply=Scene->Id==TEXT("D01_HANDOVER")?TEXT("D01_CH_NOFOOD"):Scene->Id==TEXT("D01_NOFOOD")?TEXT("D01_CH_NOFOOD_BRIEF"):FString();
   const auto Choices=E.Pack.Choices();bool Offered=false;for(const auto* Choice:Choices)Offered|=Choice&&Choice->Id==Reply;
   if(!Check(!Reply.IsEmpty()&&Offered,TEXT("actual D01 offers nofood reply; explicit offered-choice API, not mouse input")))return;
   M()->UI->Close(true);if(!Check(E.Pack.Select(Reply),TEXT("offered D01 Pack.Select production choice API")))return;E.ShowDialogue();return;
  }
  if(!Check(E.Pack.Flag(TEXT("handover_done"))&&!E.Opening()&&!M()->Story->bOpeningHeld&&M()->Story->Clock()>=S47Timeline::ShiftStart&&M()->Story->Rx.Stage==0,TEXT("actual completed D01 releases shift; no fabricated baseline/prior document")))return;
  if(!Check(M()->HorrorTest->BeginNight(Root/TEXT("physical-horror")),TEXT("night adapter adopts only the actual new D01 cohort without reinitialization")))return;
  Next(7);return;
 }
 case 7:
  if(!S47J::Bool(Adapter,TEXT("waiting"))||S47J::Str(Adapter,TEXT("boundary"))!=TEXT("sc01"))return;
  if(!Check(M()->Horror->Complete()&&M()->Horror->ReportAcknowledged()&&M()->Horror->ReferencePhotographed()&&M()->Observing->Done()&&M()->Observing->ArchiveStep()==2&&M()->Observing->GeneratorStep()==3&&M()->Observing->YardStep()==2,TEXT("actual SC01 completion/report/reference and four observing runs/chores all belong to this night")))return;
  {
   int32 Binder=0,Charts=0;for(const auto& D:M()->Story->Docs()){Binder+=D.Id==TEXT("obs_runbinder")&&!D.Page.IsEmpty();for(int32 I=0;I<FS47Observing::Runs().Num();++I)Charts+=D.Id==FS47Observing::Chart(I).Id&&!D.Page.IsEmpty();}
   if(!Check(Binder==1&&Charts==4&&M()->Night->Album->Shots().Num()==1,TEXT("real20:30binder plus four actual chart payloads and one genuine reference; no binder/film factory fixture")))return;
  }
  if(!Boundary(TEXT("sc01")))return;HeldPapers=Papers(M());
  if(!Check(M()->SaroNight&&M()->SaroNight->Dishes&&M()->SaroNight->Dishes->BeaconCount()==27,TEXT("native27 beacon baseline before fault")))return;
  for(int32 I=0;I<27;++I)
  {
   const auto C=M()->SaroNight->Dishes->BeaconBase(I);if(!Check(C.R+C.G+C.B>.0001f,TEXT("actual preoutage beacon baseline is illuminated")))return;BeaconBaseline.Add(C);
  }
  Path({FVector(4.2,0,-1.2),FVector(0,0,-1.2),FVector(0,0,-2.1)});Next(8,0);return;
 case 8:if(Walk()&&Face(TEXT("crtCenter")))Next(9,.3);return;
 case 9:
  if(!Use(TEXT("crtCenter")))return;
  {auto& Rx=M()->Story->Rx;const auto& P=S47Signal::Profiles()[1];Rx.Frequency=P.F;Rx.Gain=(P.Gain[0]+P.Gain[1])*.5;Rx.Bandwidth=(P.Bw[0]+P.Bw[1])*.5;Rx.Azimuth=(P.Az[0]+P.Az[1])*.5;M()->Story->ConsoleAzimuth();M()->Story->ConsoleAction();}
  if(!Check(M()->Story->Rx.Stage==2&&M()->Story->Phase()==ES47Phase::Survey,TEXT("actual receiver relay action starts unattended survey only after full real SC01")))return;
  {const auto W=M()->Hud->ModalWidget();if(!Check(W&&W->GetTypeAsString()==TEXT("SS47ScreenView"),TEXT("actual CRT view supplies offered EXIT API")))return;StaticCastSharedPtr<SS47ScreenView>(W)->Leave();}Next(10);return;
 case 10:
  if(!M()->Story->bResidualVisible||M()->Story->IsCinematic()||M()->ModalOpen())return;
  if(!Check(M()->Evening->FeederOutage()&&M()->Story->bDark&&M()->Story->Night().bK3Open&&!M()->Story->Rx.bPowered&&M()->Story->Rx.Stage==2&&M()->Story->Clock()>=S47Timeline::Residual(true)&&M()->Story->Clock()<900&&(!M()->Horror->Threat||!M()->Horror->Threat->Active()),TEXT("normal ConsoleClosed/survey timer creates00:13residual/feederoutage/K3out and no early alien"))||!Power(true)||!Cycle(TEXT("actual-outage-before-county-call"))||!Record(TEXT("outage")))return;
  Path({FVector(0,0,-1.2),FVector(4.2,0,-1.2),FVector(3.8,0,1.4)});Next(11,0);return;
 case 11:if(Walk()&&Face(TEXT("phone")))Next(12,.3);return;
 case 12:
  if(!Use(TEXT("phone"))||!Check(M()->ModalOpen()&&!M()->Evening->FeederCallAnswered(),TEXT("actual county phone document opens without premature acknowledgment")))return;
  M()->UI->Close(true);
  if(!Check(M()->Evening->FeederCallAnswered()&&M()->Evening->FeederOutage()&&!M()->Evening->DriveTimeSkipped()&&M()->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_FEEDER_CALL")&&!D.Page.IsEmpty();}),TEXT("actual close callback acquires county-call source and orders service trip without restoring power"))||!Cycle(TEXT("actual-county-call")))return;
  Path({FVector(4.2,0,1.6),FVector(5.2,0,1.6),FVector(7.5,0,1.6),FVector(9.3,0,1.6),FVector(9.3,0,4),FVector(10.8,0,8)});Next(13,0);return;
 case 13:
  if(!Walk())return;if(!Check(M()->HorrorTest->ResumeNight(TEXT("sc01")),TEXT("resume same actual cohort at physical service truck; native field drive/T01 rays, no direct road fixture")))return;Next(14);return;
 case 14:
  if(!S47J::Bool(Adapter,TEXT("waiting"))||S47J::Str(Adapter,TEXT("boundary"))!=TEXT("t01"))return;
  if(M()->Evening->FeederOutage()){if(Now-Mark>600)Finish(false,TEXT("actual T01 result did not restore feeder"));return;}
  if(!Check(M()->Substation->HasNormalResult()&&!M()->Story->bDark&&!M()->Story->Night().bK3Open&&M()->Story->Rx.bPowered&&M()->Evening->DriveTimeSkipped()&&M()->Story->Clock()>=S47Timeline::FeederDrive&&M()->Story->Clock()<S47Timeline::FirstEncounter,TEXT("actual signed T01 restores SARO supply/K3 after01:35drive and before02:00meeting"))||!Power(false)||!Record(TEXT("t01-restored")))return;
  if(!Check(M()->HorrorTest->ResumeNight(TEXT("t01")),TEXT("same real T01 cohort resumes actual SC02 presentation/body/capture/escape")))return;Next(15);return;
 case 15:
  if(!S47J::Bool(Adapter,TEXT("waiting"))||S47J::Str(Adapter,TEXT("boundary"))!=TEXT("sc02"))return;
  if(!Check(M()->Horror->SC02Escaped()&&M()->Substation->HasNormalResult()&&M()->World->Area==TEXT("saro")&&!M()->World->bDriving&&S47J::Bool(Adapter,TEXT("captured"))&&S47J::Num(Adapter,TEXT("sc02_clock"))>=S47Timeline::FirstEncounter,TEXT("actual02:00body/capture/local retry/runningescape/ordinary return; no copied escaped facts"))||!Boundary(TEXT("sc02")))return;
  if(!Check(M()->HorrorTest->ResumeNight(TEXT("sc02")),TEXT("actual escaped cohort enters normal SC03 report phase")))return;Next(16);return;
 case 16:
  if(!S47J::Bool(Adapter,TEXT("waiting"))||S47J::Str(Adapter,TEXT("boundary"))!=TEXT("sc03"))return;
  if(!Check(M()->Horror->SC03&&M()->Horror->SC03->Complete()&&M()->Story->Clock()>=S47Timeline::TownReport&&M()->World->Area==TEXT("diner"),TEXT("actual Caliche report/Ortiz/witnessed lift/physical refuge/traces/missing/deputy finish SC03"))||!Record(TEXT("actual-sc03-caliche-complete")))return;
  if(GoItem(TEXT("dinerTruck")))Next(17,0);return;
 case 17:if(Walk()&&Face(TEXT("dinerTruck")))Next(18,.3);return;
 case 18:
  if(!Use(TEXT("dinerTruck"),false))return;
  if(!Check(!M()->World->bCommuteVehicle&&!M()->World->bDriving&&M()->World->TruckAt==TEXT("diner"),TEXT("actual preserved service truck at diner before explicit native return-target API")))return;
  M()->World->Autopilot=10;M()->World->DriveBack();Next(19);return;
 case 19:
  if(M()->World->bDriving){if(Now-Mark>480)Finish(false,TEXT("ordinary post-SC03 return did not park at SARO"));return;}
  if(!Check(M()->World->Area==TEXT("saro")&&M()->World->TruckAt==TEXT("saro")&&M()->Horror->SC03->Complete()&&!M()->Evening->FeederOutage()&&M()->Story->Phase()==ES47Phase::Residual&&M()->Story->Rx.Stage==2&&M()->Story->bResidualVisible,TEXT("real return parks SC03-complete cohort at SARO with unsolved passive residual for SC04")))return;
  if(!Boundary(TEXT("sc03"))||!Record(TEXT("sc04-input-ready")))return;
  Finish(true,TEXT("actual newcase/commute/D01/SC01-observing/00:13/T01/SC02/SC03/SARO-return prefix complete; use its real checkpoint for SC04-SC13"));return;
 default:Finish(false,TEXT("unknown live night-prefix stage"));return;
 }
}

void S47NightContract::Step(US47Main* Main,double Dt)
{
 static TWeakObjectPtr<US47Main> Owner;static TSharedPtr<FS47NightContract> Run;
 if(Owner.Get()!=Main){Owner=Main;Run=MakeShared<FS47NightContract>(Main);}const auto Keep=Run;if(Keep)Keep->Tick(Dt);
}
