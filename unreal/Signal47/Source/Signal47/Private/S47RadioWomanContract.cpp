// HUM01: declared parked-car/clock fixtures, ordinary egress/Walker/interaction and full disk Continue.
#include "S47RadioWomanContract.h"
#include "S47Web.h"
#include "S47Caliche.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
#include "StaticMeshResources.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RadioWomanContract,Log,All);
namespace S47RadioWomanContract
{
 using FJ=S47J::FObj;
 bool Write(const FString& File,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*File);}
 bool Same(const FJ& A,const FJ& B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 double Angle(double V){return FMath::Atan2(FMath::Sin(V),FMath::Cos(V));}
 void Vec(const FJ& O,const TCHAR* Key,const FVector& P){S47J::Set(O,Key,TArray<double>{P.X,P.Y,P.Z});}
 bool Png(const FString& File,FJ& Out)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*File))return false;
  const uint8 Magic[]={137,80,78,71,13,10,26,10};if(B.Num()<33||FMemory::Memcmp(B.GetData(),Magic,8))return false;
  auto U32=[&](int64 I){return (uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;uint32 W=0,H=0;int32 Chunks=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 N=U32(At);if(int64(N)>B.Num()-At-12)return false;const uint8* T=B.GetData()+At+4;
   if(FCrc::MemCrc32(T,N+4)!=U32(At+8+N))return false;
   if(Chunks==0){if(N!=13||FMemory::Memcmp(T,"IHDR",4))return false;W=U32(At+8);H=U32(At+12);}
   if(!FMemory::Memcmp(T,"IDAT",4))Idat+=N;++Chunks;At+=int64(N)+12;
   if(!FMemory::Memcmp(T,"IEND",4)){End=N==0;break;}
  }
  if(!End||At!=B.Num()||Idat<=0||W<320||H<200)return false;
  FSHAHash Hash;FSHA1::HashBuffer(B.GetData(),B.Num(),Hash.Hash);Out=S47J::New();
  S47J::Set(Out,TEXT("file"),File);S47J::Set(Out,TEXT("sha1"),Hash.ToString());S47J::Set(Out,TEXT("bytes"),B.Num());
  S47J::Set(Out,TEXT("width"),double(W));S47J::Set(Out,TEXT("height"),double(H));
  S47J::Set(Out,TEXT("all_chunk_crc_verified"),true);S47J::Set(Out,TEXT("iend_at_eof"),true);return true;
 }
 struct FControl:TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;FTimerHandle Timer;FString Root,PhotoName,SaveId;
  bool Done=false,Started=false,Requested=false,Fallback=false;
  int32 Stage=0,Checks=0,Waypoint=0,Schedule=0,NativeNode=INDEX_NONE;
  double Began=FPlatformTime::Seconds(),At=Began,Ready=0,PhotoAt=0,Last=Began,WalkMetres=0,ProgressAt=Began;
  FVector LastFoot=FVector::ZeroVector,ProgressFoot=FVector::ZeroVector,SavedFoot=FVector::ZeroVector;
  double SavedYaw=0,SavedPitch=0,SavedClock=0;TMap<FString,bool> Doors;TArray<FVector> Path;
  FJ BeforeSave,PackBefore;TArray<FString> SeenBefore;
  TArray<TSharedPtr<FJsonValue>> Photos,Snapshots,Events;
  US47Main* Main()const{return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  UStaticMeshComponent* Body()const{const auto* N=Web->Node(NativeNode);return N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;}
  void Result(bool Pass,const FString& Why)
  {
   const auto Keep=AsShared();if(Done)return;Done=true;auto* M=Main();
   if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(Body())Body()->SetForcedLodModel(0);
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("walk_metres"),WalkMetres);S47J::Set(O,TEXT("explicit_off_fallback"),Fallback);
   O->SetArrayField(TEXT("snapshots"),Snapshots);O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("events"),Events);
   S47J::Set(O,TEXT("scope"),TEXT("Explicit saved-car start road x=-6 z=2138, then ordinary GetOutHere/Walker/Inter.Pick and UseId. Caliche.Choice is API-driven. Declared clock fixtures exercise actual TownStep schedule. Actual case0 snapshot copied unchanged to isolated case1 disk for ordinary title/Continue. Human input, listening, FPS, whole night/full matrix and installation unverified."));
   const bool Written=Write(Root/TEXT("result.json"),O);if(Web.IsValid())Web->GetWorldTimerManager().ClearTimer(Timer);
   if(!Written){UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL stage=%d: final result could not be written: %s"),Stage,*Root);FPlatformMisc::RequestExitWithStatus(false,1);return;}
   if(Pass){UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioWoman contract PASS checks=%d walk=%.3fm evidence=%s"),Checks,WalkMetres,*Root);FPlatformMisc::RequestExit(false);}
   else{UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL stage=%d: %s evidence=%s"),Stage,*Why,*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Good,const FString& Why){++Checks;if(!Good){Result(false,Why);return false;}return true;}
  void Next(int32 S,const FString& Why){Stage=S;At=FPlatformTime::Seconds();auto E=S47J::New();S47J::Set(E,TEXT("stage"),S);S47J::Set(E,TEXT("reason"),Why);Events.Add(MakeShared<FJsonValueObject>(E));UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47 RadioWoman stage %d %s"),S,*Why);}
  FVector Target()const{return Web->Obj(TEXT("world.town.proxies.woman")).WorldPosition();}
  void Look(double Raise=0){auto* M=Main();const auto D=Target()+FVector(0,Raise,0)-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
  bool ShoeContact(const FJ& Snapshot)
  {
   auto* C=Body();const auto* Mesh=C?C->GetStaticMesh().Get():nullptr;
   if(!Check(Mesh&&Mesh->bAllowCPUAccess&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==3,TEXT("actual static shoe geometry available for independent ground contact rays")))return false;
   const auto& L=Mesh->GetRenderData()->LODResources[0];TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);TSet<uint32> Candidates[2];double Low[2]={1e20,1e20};
   for(const auto& S:L.Sections)
   {
    if(!Mesh->GetStaticMaterials().IsValidIndex(S.MaterialIndex))continue;const auto* Mat=Mesh->GetMaterial(S.MaterialIndex);if(!Mat||(Mat->GetName()!=TEXT("M_RW_Shoe")&&Mat->GetName()!=TEXT("M_RW_Trim")))continue;
    for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;++I)
    {
     if(I>=uint32(Indices.Num())||Indices[I]>=L.VertexBuffers.PositionVertexBuffer.GetNumVertices()){Result(false,TEXT("shoe section index outside actual CPU vertex payload"));return false;}
     const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[I]));if(P.Z>=-15)continue;
     const int32 Side=P.Y<0?0:1;Candidates[Side].Add(Indices[I]);Low[Side]=FMath::Min(Low[Side],P.Z);
    }
   }
   if(!Check(Candidates[0].Num()>=3&&Candidates[1].Num()>=3,TEXT("both shoes have actual lowest-geometry candidates in native Shoe/Trim material sections")))return false;
   TArray<TSharedPtr<FJsonValue>> Feet;
   for(int32 Side=0;Side<2;++Side)
   {
    int32 Rays=0;double Min=1e20,Max=-1e20;TArray<TSharedPtr<FJsonValue>> Samples;
    TArray<FVector> Unique;
    for(uint32 Index:Candidates[Side])
    {
     const FVector P(L.VertexBuffers.PositionVertexBuffer.VertexPosition(Index));if(P.Z<=Low[Side]+.15&&!Unique.ContainsByPredicate([&](const FVector& V){return V.Equals(P,.00001);}))Unique.Add(P);
    }
    if(!Check(Unique.Num()>=3&&Unique.Num()<=256,TEXT("contact sample includes every unique lowest sole vertex, without duplicate-index or first16 bias")))return false;
    for(const FVector& P:Unique)
    {
     const FVector World=C->GetComponentTransform().TransformPosition(P);FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanSole),true,Main()->Player.Get());
     const bool Hit=Web->GetWorld()->LineTraceSingleByChannel(H,World+FVector(0,0,50),World-FVector(0,0,100),ECC_Visibility,Q);
     if(!Check(Hit&&H.ImpactNormal.Z>.7,TEXT("actual ground under each sampled lowest sole vertex")))return false;
     const double Gap=World.Z-H.ImpactPoint.Z;Min=FMath::Min(Min,Gap);Max=FMath::Max(Max,Gap);++Rays;
     auto R=S47J::New();Vec(R,TEXT("sole_world_ue_cm"),World);Vec(R,TEXT("ground_world_ue_cm"),H.ImpactPoint);S47J::Set(R,TEXT("gap_cm"),Gap);S47J::Set(R,TEXT("ground_component"),GetPathNameSafe(H.GetComponent()));Samples.Add(MakeShared<FJsonValueObject>(R));
    }
    auto F=S47J::New();S47J::Set(F,TEXT("side"),Side);S47J::Set(F,TEXT("ground_rays"),Rays);S47J::Set(F,TEXT("unique_lowest_sole_vertices"),Unique.Num());S47J::Set(F,TEXT("minimum_gap_cm"),Min);S47J::Set(F,TEXT("maximum_gap_cm"),Max);F->SetArrayField(TEXT("samples"),Samples);Feet.Add(MakeShared<FJsonValueObject>(F));
    if(!Check(Rays>=3&&Min>=-1.5&&Max<=1.5,TEXT("actual seated shoe contact within1.5cm of rendered ground, without body scale or collision changes")))
    {Snapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);Write(Root/TEXT("first-shoe-contact-failure.json"),Snapshot);return false;}
   }
   Snapshot->SetArrayField(TEXT("shoe_ground_contact"),Feet);return true;
  }
  bool Snapshot(const FString& Name,bool Expected)
  {
   auto* M=Main();const auto Woman=Web->Obj(TEXT("world.town.woman")),Proxy=Web->Obj(TEXT("world.town.proxies.woman"));
   if(!Check(Woman.I==1480&&Proxy.I==1544&&M->TownLayer->Town->bWomanIn==Expected&&Woman.Visible()==Expected&&Proxy.Visible()==Expected&&Proxy.Shown()==Expected,TEXT("original woman group/proxy and actual normal TownStep share expected presence: ")+Name))return false;
   auto O=S47J::New();S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("clock_seconds"),M->Story->Clock());S47J::Set(O,TEXT("expected_present"),Expected);
   Vec(O,TEXT("player_three_m"),M->Player->Pos);Vec(O,TEXT("proxy_three_m"),Proxy.WorldPosition());
   S47J::Set(O,TEXT("native_readback"),Web->RadioWomanReadback());
   TArray<FString> Lines;const bool Audit=Web->AuditRadioWoman(Lines);S47J::Set(O,TEXT("audit_lines"),Lines);S47J::Set(O,TEXT("audit_pass"),Audit);
   if(Fallback)
   {
    if(!Check(!Audit&&NativeNode==INDEX_NONE,TEXT("explicit Off leaves no mannequin or partial body replacement")))return false;
    if(!Check(S47J::Bool(Web->RadioWomanReadback(),TEXT("fallback_original_bindings_intact")),TEXT("explicit Off leaves all actual original mesh/material/transform/collision bindings intact")))return false;
    for(int32 I=1481;I<=1486;++I)if(!Check(Web->Node(I)&&Web->ObjAt(I).Visible(),TEXT("explicit Off retains each complete original body/radio leaf")))return false;
   }
   else
   {
    if(!Check(Audit&&Body()&&Body()->IsVisible()==Expected&&Web->ObjAt(NativeNode).Shown()==Expected,TEXT("native static body follows existing parent presence and full adapter audit")))return false;
    for(int32 I:{1481,1482,1483,1486})if(!Check(!Web->ObjAt(I).Visible(),TEXT("old body-only leaf remains retired without double body")))return false;
    if(!Check(Web->ObjAt(1484).Visible()&&Web->ObjAt(1485).Visible(),TEXT("original split radio housing and metal panel remain enabled")))return false;
    if(!Check(!Body()->IsComponentTickEnabled()&&Body()->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("posed figure has no animation/tick or extra gameplay blocker")))return false;
    if((Name==TEXT("arrival-present")||Name==TEXT("after-continue-present"))&&!ShoeContact(O))return false;
   }
   if(!Check(Expected?!M->Inter->LabelOf(TEXT("town:woman")).IsEmpty():M->Inter->LabelOf(TEXT("town:woman")).IsEmpty(),TEXT("original woman interaction label agrees with actual presence")))return false;
   const auto Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());S47J::Set(O,TEXT("actual_view_ray_pick"),Pick);
   if(!Fallback&&Name.StartsWith(TEXT("clock-fixture-"))&&!Check(Expected?Pick==TEXT("town:woman"):Pick!=TEXT("town:woman"),TEXT("actual ray can select woman only while original proxy is present")))return false;
   if(!Write(Root/(Name+TEXT(".json")),O)){Result(false,TEXT("snapshot write failed"));return false;}Snapshots.Add(MakeShared<FJsonValueObject>(O));return true;
  }
  void QueuePhoto(const FString& Name){PhotoName=Name;Requested=false;PhotoAt=FPlatformTime::Seconds();}
  bool Photo()
  {
   auto* M=Main();const double Now=FPlatformTime::Seconds();Look(.25);const auto File=Root/(PhotoName+TEXT(".png"));
   if(!Requested){if(Now-PhotoAt<.6)return false;FScreenshotRequest::RequestScreenshot(File,true,false);Requested=true;return false;}
   FJ R;if(!Png(File,R)){if(Now-PhotoAt>12)Result(false,TEXT("complete viewport PNG missing: ")+PhotoName);return false;}
   S47J::Set(R,TEXT("name"),PhotoName);Vec(R,TEXT("player_three_m"),M->Player->Pos);Vec(R,TEXT("eye_ue_cm"),M->Player->EyeUE());Vec(R,TEXT("look_direction_ue"),M->Player->ForwardUE());
   if(auto* PC=Cast<APlayerController>(M->Player->GetController()))if(PC->PlayerCameraManager){S47J::Set(R,TEXT("fov"),PC->PlayerCameraManager->GetFOVAngle());Vec(R,TEXT("camera_ue_cm"),PC->PlayerCameraManager->GetCameraLocation());}
   S47J::Set(R,TEXT("clock_seconds"),M->Story->Clock());S47J::Set(R,TEXT("forced_lod_for_inspection"),Body()?Body()->GetForcedLodModel():0);
   if(!Write(Root/(PhotoName+TEXT(".capture.json")),R)){Result(false,TEXT("capture metadata write failed"));return false;}Photos.Add(MakeShared<FJsonValueObject>(R));return true;
  }
  bool Floor()
  {
   auto* P=Main()->Player.Get();FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RadioWomanFloor),true,P);
   const bool Hit=Web->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(P->Pos.X,P->FloorY+.12,P->Pos.Z),S47Three::ToUE(P->Pos.X,P->FloorY-.35,P->Pos.Z),ECC_Visibility,Q);
   return Check(P->Walkable(P->Pos.X,P->Pos.Z)&&Hit&&FMath::Abs(S47Three::ToThree(H.ImpactPoint).Y-P->FloorY)<.08,TEXT("ordinary Walker and actual rendered ground at each route waypoint"));
  }
  bool Walk(double Now)
  {
   auto* P=Main()->Player.Get();const double Moved=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();
   if(Moved>FMath::Max(.5,(Now-Last)*8+.1)){Result(false,TEXT("Walker discontinuity; no placement recovery"));return false;}
   WalkMetres+=Moved;LastFoot=P->Pos;Last=Now;
   if(FVector2D(P->Pos.X-ProgressFoot.X,P->Pos.Z-ProgressFoot.Z).Size()>.15){ProgressFoot=P->Pos;ProgressAt=Now;}
   if(Waypoint>=Path.Num()){P->TestMove.Reset();return true;}
   const auto D=Path[Waypoint]-P->Pos;const double Distance=FVector2D(D.X,D.Z).Size();
   if(Distance<.085){P->TestMove.Reset();if(!Floor())return false;++Waypoint;return false;}
   if(Now-ProgressAt>12){P->TestMove.Reset();Result(false,TEXT("ordinary route stalled before bench; no pose or tolerance fallback"));return false;}
   P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=-.08;P->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.5));P->bTestRun=false;return false;
  }
  void Tick()
  {
   if(Done)return;const double Now=FPlatformTime::Seconds();if(Now-Began>180){Result(false,TEXT("targeted journey timeout"));return;}
   auto* M=Main();if(!M||!M->Flow||!M->Player||!M->World||!M->Story||!M->UI||!M->Inter||!M->Evening||!M->TownLayer||!M->TownLayer->Town||!M->TownLayer->Caliche||S47Startup::Blocking()){Ready=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){Ready=0;return;}if(Ready==0){Ready=Now;return;}if(Now-Ready<.6)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Evening->Commute()&&M->World->bDriving,TEXT("ordinary fresh commute before explicit parked-car/clock fixture")))return;
    for(int32 I=0;I<Web->PartCount();++I)if(Web->Node(I)->Name==TEXT("mannequin:radio-woman")){if(!Check(NativeNode==INDEX_NONE,TEXT("exactly one adopted mannequin node")))return;NativeNode=I;}
    if(!Fallback&&!Check(Body()&&Body()->GetStaticMesh()&&Body()->GetStaticMesh()->GetRenderData()&&Body()->GetStaticMesh()->GetRenderData()->LODResources.Num()==3,TEXT("actual native three-LOD mannequin exists before test fixtures")))return;
    PackBefore=S47J::Obj(S47J::Obj(M->Flow->GameState(),TEXT("evening")),TEXT("pack"));
    auto Car=S47J::New();S47J::Set(Car,TEXT("leg"),TEXT("road"));S47J::Set(Car,TEXT("x"),7994.);S47J::Set(Car,TEXT("y"),0.);S47J::Set(Car,TEXT("z"),2138.);S47J::Set(Car,TEXT("h"),0.);
    S47J::Set(Car,TEXT("scope"),TEXT("only explicit position fixture; no later car or player pose writes"));Write(Root/TEXT("explicit-start-fixture.json"),Car);
    Started=true;M->World->BeginCommute(Car);M->Story->SetClock(-19800);Next(1,TEXT("explicit parked-car fixture at west shoulder and clock18:30; wait ordinary handover"));return;
   }
   if(M->Flow->bStarting||M->World->bBusy)return;
   if(Stage==1)
   {
    if(Now-At<.8)return;if(!Check(M->World->CanGetOut()&&M->World->GetOutHere(),TEXT("ordinary GetOutHere from declared parked commute start")))return;Next(2,TEXT("normal egress; no foot placement"));return;
   }
   if(Stage==2)
   {
    if(Now-At<.8)return;if(!Check(!M->World->bDriving&&M->World->Area==TEXT("diner"),TEXT("actual Caliche walking area after parking")))return;
    if(!Snapshot(TEXT("arrival-present"),true)||!Floor())return;
    if(Fallback){Next(11,TEXT("explicit Off: original scene/presence only, no native journey acceptance"));return;}
    const auto T=Target();const auto P=M->Player->Pos;Path={FVector(T.X+2.05,P.Y,P.Z),FVector(T.X+2.05,P.Y,T.Z)};
    for(const auto& Q:Path)if(!Check(M->Player->Walkable(Q.X,Q.Z),TEXT("bench approach remains in original walkable area")))return;
    LastFoot=ProgressFoot=P;Last=ProgressAt=Now;Next(3,TEXT("ordinary continuous Walker approach to original bench proxy"));return;
   }
   if(Stage==3){if(!Walk(Now))return;Look();Body()->SetForcedLodModel(1);QueuePhoto(TEXT("bench-lod0"));Next(4,TEXT("actual close LOD0 pose/radio/seat view"));return;}
   if(Stage==4){if(!Photo())return;Body()->SetForcedLodModel(2);QueuePhoto(TEXT("bench-lod1"));Next(5,TEXT("same actual camera, authored LOD1 inspection"));return;}
   if(Stage==5){if(!Photo())return;Body()->SetForcedLodModel(3);QueuePhoto(TEXT("bench-lod2"));Next(6,TEXT("same actual camera, authored LOD2 inspection"));return;}
   if(Stage==6)
   {
    if(!Photo())return;Body()->SetForcedLodModel(0);Look();Next(14,TEXT("allow actual camera to settle on interaction proxy after inspection"));return;
   }
   if(Stage==14)
   {
    Look();if(Now-At<.35)return;
    if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("town:woman"),TEXT("actual view ray selects unchanged original woman proxy")))return;
    M->Inter->UseId(TEXT("town:woman"));Next(7,TEXT("original interaction opens existing radio-woman conversation"));return;
   }
   if(Stage==7)
   {
    if(Now-At<.5)return;if(!Check(M->UI->Modal()&&M->TownSeen.Contains(TEXT("met:radio_woman")),TEXT("actual original conversation modal and met flag")))return;
    M->TownLayer->Caliche->Choice(TEXT("radio"));QueuePhoto(TEXT("radio-conversation"));Next(8,TEXT("API choice exercises original listening-topic state"));return;
   }
   if(Stage==8)
   {
    if(!Photo())return;if(!Check(M->TownSeen.Contains(TEXT("ask:radio_woman:radio")),TEXT("existing radio-topic flag written by original conversation")))return;
    M->UI->Close();Next(9,TEXT("close actual modal before normal save boundary"));return;
   }
   if(Stage==9)
   {
    if(Now-At<.8)return;if(!Check(WalkMetres>2&&M->Flow->GameSaveBlock().IsEmpty(),TEXT("real bench approach complete and ordinary gameplay save boundary")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();Doors=Web->DoorStates();SeenBefore=M->TownSeen.Array();
    if(!Check(Same(PackBefore,S47J::Obj(S47J::Obj(BeforeSave,TEXT("evening")),TEXT("pack"))),TEXT("bench interaction preserved full commute quest pack")))return;
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("HUM01 radio woman");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;FString Error;
    const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT DISK FIXTURE: full actual case0 snapshot copied to isolated case1: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),BeforeSave),TEXT("fresh reader equals entire actual disk snapshot")))return;
    Write(Root/TEXT("before-continue.disk.json"),BeforeSave);SaveId=Meta->Id;M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(10,TEXT("ordinary title then full Continue lifecycle"));return;
   }
   if(Stage==10)
   {
    if(M->Mode==US47Main::EMode::Title){if(Now-At<.6)return;const auto S=M->Flow->Saves.ContinueSave();if(!Check(S&&S->Id==SaveId,TEXT("ordinary title resolves exact isolated disk save")))return;M->Flow->OpenTestView(TEXT("continue"));return;}
    if(Now-At<1||M->Mode!=US47Main::EMode::Play||!M->Flow->SaveBlock().IsEmpty())return;
    const auto Actual=M->Flow->GameState();const auto A=S47J::Obj(S47J::Obj(BeforeSave,TEXT("evening")),TEXT("car")),B=S47J::Obj(S47J::Obj(Actual,TEXT("evening")),TEXT("car"));
    bool Okay=M->Evening->Commute()&&!M->World->bDriving&&M->World->Area==TEXT("diner")&&M->Player->Pos.Equals(SavedFoot,.015)&&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Story->Clock()-SavedClock)<.001;
    for(const auto& P:Doors)Okay&=Web->DoorStates().FindRef(P.Key)==P.Value;
    for(const auto& K:SeenBefore)Okay&=M->TownSeen.Contains(K);
    for(const TCHAR* K:{TEXT("x"),TEXT("y"),TEXT("z"),TEXT("h")})Okay&=A&&B&&FMath::Abs(S47J::Num(A,K)-S47J::Num(B,K))<.015;
    Okay&=Same(PackBefore,S47J::Obj(S47J::Obj(Actual,TEXT("evening")),TEXT("pack")));
    auto R=S47J::New();S47J::Set(R,TEXT("expected"),BeforeSave);S47J::Set(R,TEXT("actual"),Actual);S47J::Set(R,TEXT("pass"),Okay);Write(Root/TEXT("continue-readback.json"),R);
    if(!Check(Okay,TEXT("full ordinary Flow Continue retained player/car/clock/doors/quest pack and conversation flags"))||!Snapshot(TEXT("after-continue-present"),true))return;
    QueuePhoto(TEXT("after-continue"));Next(12,TEXT("same actual placement after full disk Continue"));return;
   }
   if(Stage==12){if(!Photo())return;Next(11,TEXT("declared clock fixtures exercise actual schedule through ordinary frames"));return;}
   if(Stage==11)
   {
    const double Clocks[]={-21601.,-21600.,17399.,17400.,-19800.};
    if(Schedule<5)
    {
     if(Now-At<.3)return;M->Story->SetClock(Clocks[Schedule]);Look();Next(13,TEXT("explicit clock fixture; wait actual TownStep, never SetPeople"));return;
    }
    if(Fallback){Result(true,TEXT("explicit Off preserves all original body/radio leaves and existing presence/proxy schedule"));return;}
    FString Error;const auto Actual=M->Flow->GameState();const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary Flow.WriteSave after restored schedule: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();if(!Check(Same(Reader.Load(Meta->Id),Actual),TEXT("fresh reader equals entire normal post-Continue save")))return;
    for(const auto& P:Photos){FJ R;if(!Check(Png(S47J::Str(P->AsObject(),TEXT("file")),R)&&S47J::Str(R,TEXT("sha1"))==S47J::Str(P->AsObject(),TEXT("sha1")),TEXT("final original viewport PNG CRC/IEND/hash verified")))return;}
    Result(true,TEXT("native static three-LOD figure, preserved original radio/proxy, physical bench approach/conversation, actual presence boundaries and full disk/title/Continue"));return;
   }
   if(Stage==13)
   {
    const bool Present[]={false,true,true,false,true};if(Now-At<.35)return;
    if(!Snapshot(FString::Printf(TEXT("clock-fixture-%d"),Schedule),Present[Schedule]))return;++Schedule;Next(11,TEXT("actual schedule readback complete"));return;
   }
  }
 };
}
void S47BeginRadioWomanContract(AS47Web* Web)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanContract")))return;
 FString UserDir,Evening;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 bool Conflict=false;for(const TCHAR* P:{TEXT("S47ProductionViews"),TEXT("S47FarmAccessContract"),TEXT("S47CalicheHomesContract"),TEXT("S47MotelV2Route"),TEXT("S47Title"),TEXT("S47SmokeTest"),TEXT("S47HorrorArrival"),TEXT("S47HorrorContract")})Conflict|=FParse::Param(FCommandLine::Get(),P);
 const bool Fallback=FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanContractFallback"));
 if(Conflict||UserDir.Contains(TEXT(".."))||!(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))||!FParse::Value(FCommandLine::Get(),TEXT("S47Evening="),Evening)||Evening!=TEXT("commute")||(Fallback&&!FParse::Param(FCommandLine::Get(),TEXT("S47RadioWomanOff"))))
 {UE_LOG(LogS47RadioWomanContract,Error,TEXT("S47: RadioWoman contract FAIL requires isolated /tmp profile, commute, no competing driver; fallback requires explicit Off"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto S=MakeShared<S47RadioWomanContract::FControl>();S->Web=Web;S->Fallback=Fallback;
 S->Root=FPaths::ProjectSavedDir()/TEXT("RadioWomanContract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*S->Root,true)){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 Web->GetWorldTimerManager().SetTimer(S->Timer,[S]{S->Tick();},.05f,true);
 UE_LOG(LogS47RadioWomanContract,Display,TEXT("S47: RadioWoman contract armed evidence=%s"),*S->Root);
}
