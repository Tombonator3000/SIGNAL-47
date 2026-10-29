// Targeted real drive + original finale. Test fixtures and actual captures are kept distinct.
#include "S47SC13Contract.h"
#include "S47Chapter6.h"
#include "S47Chapter5.h"
#include "S47Diner.h"
#include "S47FinaleSky.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Sky.h"
#include "S47SkyVisitors.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "UnrealClient.h"

class FS47SC13Contract : public TSharedFromThis<FS47SC13Contract>
{
 using FJ=S47J::FObj;
 TWeakObjectPtr<US47Main> Main;
 FString Root,Mode,Source,PhotoBefore,PendingPhoto;
 FJ Departure,SourceResult,PauseBefore,PendingFrame;
 TOptional<FS47SaveMeta> SaveMeta;
 int32 Stage=0,Checks=0,Waypoint=0,RestoreCount=0;
 bool Done=false,Armed=false,PhotoRequested=false,DidPause=false,DidKey=false,DidDoor=false,LinePlaced=false;
 double Start=FPlatformTime::Seconds(),Since=Start,LastWall=Start,EndAt=0,WalkMetres=0,DriveMetres=0,MaxMile=0,EventAt=0,PullAt=0;
 FVector LastFoot=FVector::ZeroVector,LastCar=FVector::ZeroVector;
 bool HaveCar=false;
 TArray<FVector> Path;
 TSet<FString> Shots;
 TArray<TSharedPtr<FJsonValue>> Evidence,Photos,Samples;
 US47Main* M()const{return Main.Get();}
 US47Chapter6* C()const{return M()&&M()->Story?Cast<US47Chapter6>(M()->Story->Chapter(TEXT("ch6"))):nullptr;}
 US47Chapter5* C5()const{return M()&&M()->Story?Cast<US47Chapter5>(M()->Story->Chapter(TEXT("ch5"))):nullptr;}
 bool Write(const FString& Name,const FJ& O)const{return FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/Name));}
 static bool Same(const FJ&A,const FJ&B){return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 static FJ Read(const FString& File){FString S;return FFileHelper::LoadFileToString(S,*File)?S47J::Read(S):FJ();}
 static FString FileHash(const FString& File){TArray<uint8>B;if(File.IsEmpty()||!FFileHelper::LoadFileToArray(B,*File))return {};FSHAHash H;FSHA1::HashBuffer(B.GetData(),B.Num(),H.Hash);return H.ToString();}
 static bool CompletePng(const TArray<uint8>& B)
 {
  const uint8 Sig[]={137,80,78,71,13,10,26,10};if(B.Num()<33||FMemory::Memcmp(B.GetData(),Sig,8))return false;
  const auto U32=[&](int64 I){return(uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;int32 Chunks=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>B.Num()-At-12)return false;const auto* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size))return false;
   if(!Chunks&&(FMemory::Memcmp(Type,"IHDR",4)||Size!=13||U32(At+8)<640||U32(At+12)<360))return false;
   if(!FMemory::Memcmp(Type,"IDAT",4))Idat+=Size;++Chunks;At+=Size+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  return End&&At==B.Num()&&Idat>0;
 }
 FString PhotoHash()const{for(const auto* E:{TEXT("png"),TEXT("jpg")}){const FString P=FPaths::ProjectContentDir()/TEXT("WebData/art")/(FString(TEXT("photo1947."))+E);if(IFileManager::Get().FileExists(*P))return FileHash(P);}return {};}
 void Result(bool Pass,const FString& Why)
 {
  const auto Keep=AsShared();if(Done)return;Done=true;
  if(M()&&M()->Player)M()->Player->TestMove.Reset();if(M()&&M()->World)M()->World->Autopilot.Reset();
  auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("mode"),Mode);S47J::Set(O,TEXT("checks"),Checks);
  S47J::Set(O,TEXT("sourceSC12"),SourceResult);S47J::Set(O,TEXT("sourceDepartureSHA1"),FileHash(Source));S47J::Set(O,TEXT("ordinaryDiskContinues"),RestoreCount);
  S47J::Set(O,TEXT("walkMetres"),WalkMetres);S47J::Set(O,TEXT("driveMetres"),DriveMetres);S47J::Set(O,TEXT("maximumMile"),MaxMile);S47J::Set(O,TEXT("eventWallSeconds"),PullAt-EventAt);
  S47J::Set(O,TEXT("pullWallSeconds"),EndAt-PullAt);S47J::Set(O,TEXT("originalPhotoSHA1"),PhotoBefore);S47J::Set(O,TEXT("currentPhotoSHA1"),PhotoHash());
  S47J::Set(O,TEXT("routeScope"),Mode==TEXT("full")?TEXT("FULL_CONTIGUOUS_DINER_TO_C"):TEXT("DECLARED_AT_LINE_REGRESSION_ONLY"));
  S47J::Set(O,TEXT("source"),TEXT("Actual SC12 departure and ordinary disk/title/Continue for revised full/line. Legacy is explicit unmodified developer last-line fixture."));
  S47J::Set(O,TEXT("visualHumanAcceptance"),false);S47J::Set(O,TEXT("humanInput"),false);S47J::Set(O,TEXT("installation"),false);
  S47J::Set(O,TEXT("lastReadback"),C()?C()->FinaleReadback():FJ());O->SetArrayField(TEXT("checksExecuted"),Evidence);O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("frameSamples"),Samples);
  const bool Written=!Root.IsEmpty()&&Write(TEXT("result.json"),O);
  if(Pass&&Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc13 PASS COMPLETE: %s %d checks, %.3fm drive, %d disk Continues"),*Mode,Checks,DriveMetres,RestoreCount);FPlatformMisc::RequestExitWithStatus(false,0);}
  else{UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc13 FAIL stage %d: %s"),Stage,*Why);FPlatformMisc::RequestExitWithStatus(false,1);}
 }
 bool Check(bool Good,const FString& Why)
 {
  ++Checks;auto R=S47J::New();S47J::Set(R,TEXT("pass"),Good);S47J::Set(R,TEXT("detail"),Why);S47J::Set(R,TEXT("stage"),Stage);Evidence.Add(MakeShared<FJsonValueObject>(R));if(!Good)Result(false,Why);return Good;
 }
 void Next(int32 N){Stage=N;Since=FPlatformTime::Seconds();}
 bool Snapshot(const FString& Name)
 {
  auto R=C()->FinaleReadback();S47J::Set(R,TEXT("name"),Name);S47J::Set(R,TEXT("gameState"),M()->Flow->GameState());
  if(M()->SkyVisitors)S47J::Set(R,TEXT("skyVisitors"),M()->SkyVisitors->Save());
  if(M()->Life&&M()->Life->Sky){S47J::Set(R,TEXT("randomMeteors"),M()->Life->Sky->Meteors);S47J::Set(R,TEXT("randomFlash"),M()->Life->Sky->Flash);}
  return Check(Write(Name+TEXT(".json"),R),TEXT("fresh raw snapshot: ")+Name);
 }
 bool Picture(const FString& Name)
 {
  if(Shots.Contains(Name))return true;
  const FString File=Root/(Name+TEXT(".png"));
  if(PendingPhoto.IsEmpty())
  {
   if(!Check(!FScreenshotRequest::IsScreenshotRequested()&&!IFileManager::Get().FileExists(*File),TEXT("own fresh screenshot request: ")+Name))return false;
   PendingPhoto=Name;PendingFrame=C()->FinaleReadback();S47J::Set(PendingFrame,TEXT("requestFrame"),double(GFrameCounter));PhotoRequested=true;FScreenshotRequest::RequestScreenshot(File,true,false);return false;
  }
  if(PendingPhoto!=Name)return false;
  TArray<uint8>Bytes;
  if(!FFileHelper::LoadFileToArray(Bytes,*File)||!CompletePng(Bytes))return false;
  if(!Check(true,TEXT("actual viewport PNG complete CRC/IEND/resolution: ")+Name))return false;
  auto R=PendingFrame;S47J::Set(R,TEXT("name"),Name);S47J::Set(R,TEXT("completedFrame"),double(GFrameCounter));S47J::Set(R,TEXT("path"),File);S47J::Set(R,TEXT("bytes"),Bytes.Num());S47J::Set(R,TEXT("sha1"),FileHash(File));S47J::Set(R,TEXT("humanVisualAcceptance"),false);Photos.Add(MakeShared<FJsonValueObject>(R));
  Shots.Add(Name);PendingPhoto.Reset();PhotoRequested=false;return true;
 }
 void PathToTruck()
 {
  const auto G=M()->TownLayer->Diner->Group;Path={G.LocalToWorld(FVector(-1.4,0,7.25)),G.LocalToWorld(FVector(-1.4,0,0)),G.LocalToWorld(FVector(3.2,0,0)),G.LocalToWorld(FVector(7,0,0))};
  if(const auto* T=M()->World->TruckHere())Path.Add(T->LocalToWorld(FVector(-2.1,0,.1)));
  Waypoint=0;LastFoot=M()->Player->Pos;
 }
 bool Walk(double Dt)
 {
  auto* P=M()->Player.Get();const double Moved=FVector2D(P->Pos.X-LastFoot.X,P->Pos.Z-LastFoot.Z).Size();
  if(!Check(Moved<=FMath::Max(.7,Dt*4+.2),TEXT("no discontinuity in ordinary diner-to-truck Walker")))return false;
  WalkMetres+=Moved;LastFoot=P->Pos;
  if(Waypoint>=Path.Num()){P->TestMove.Reset();return true;}
  const FVector D=Path[Waypoint]-P->Pos;const double N=FVector2D(D.X,D.Z).Size();if(N<.12){++Waypoint;P->TestMove.Reset();return false;}
  P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=-.05;P->bTestRun=false;P->TestMove=FVector2D(0,-FMath::Min(1.,N/.4));return false;
 }
public:
 explicit FS47SC13Contract(US47Main* InMain):Main(InMain){}
 void Tick(double Dt)
 {
  if(Done)return;const double Now=FPlatformTime::Seconds(),WallDt=Now-LastWall;LastWall=Now;
  if(!M()||!M()->Flow||!M()->World||!M()->Story||!M()->Player||!M()->Web||!M()->Inter||!M()->UI||!C()||!C5())return;
  if(Now-Start>1150){Result(false,TEXT("SC13 physical journey timeout; no teleport/clock recovery"));return;}
  if(Stage==0)
  {
   if(M()->Mode!=US47Main::EMode::Play||M()->Flow->bStarting||M()->World->bBusy||Now-Start<3)return;
   FString UserDir;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::NormalizeDirectoryName(UserDir);
   Root=FPaths::ProjectSavedDir()/TEXT("SpineSC13Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
   if(!IFileManager::Get().MakeDirectory(*Root,true)){Result(false,TEXT("fresh evidence directory unavailable"));return;}
   FParse::Value(FCommandLine::Get(),TEXT("S47SC13Mode="),Mode);if(Mode.IsEmpty())Mode=TEXT("full");
   if(!Check(UserDir.StartsWith(TEXT("/tmp/"))&&!UserDir.Contains(TEXT(".."))&&M()->Flow->bDevRun&&M()->Flow->CaseId==0&&(Mode==TEXT("full")||Mode==TEXT("line")||Mode==TEXT("legacy")),TEXT("explicit isolated developer SC13 mode/profile")))return;
   // Exercise the existing settings callbacks in this isolated profile; no product flag is forced.
   const auto Settings=M()->Flow->Settings();
   if(FParse::Param(FCommandLine::Get(),TEXT("S47SC13Comfort"))){if(Settings.OnCalmFlash)Settings.OnCalmFlash(true);if(Settings.OnStillShots)Settings.OnStillShots(true);}
   PhotoBefore=PhotoHash();if(!Check(!PhotoBefore.IsEmpty(),TEXT("actual original photo1947 bytes available before finale")))return;
   Armed=true;
   if(Mode==TEXT("legacy")){if(!Check(C()->bActive&&!C()->RevisedFinale(),TEXT("legacy last-line fixture remains original without new route facts")))return;Next(4);return;}
   FParse::Value(FCommandLine::Get(),TEXT("S47SC13Departure="),Source);Source=FPaths::ConvertRelativePathToFull(Source);
   Departure=Read(Source);SourceResult=Read(FPaths::GetPath(Source)/TEXT("result.json"));
   const auto Case=S47J::Obj(Departure,TEXT("case")),Ch5=S47J::Obj(Case,TEXT("ch5")),R=S47J::Obj(Ch5,TEXT("sc12"));
   if(!Check(S47J::Str(SourceResult,TEXT("status"))==TEXT("PASS")&&S47J::Int(SourceResult,TEXT("ordinaryRayUses"))>=6&&S47J::Int(SourceResult,TEXT("diskFlowRestores"))>=8
    &&S47J::Str(Departure,TEXT("checkpoint"))==TEXT("chapter5")&&S47J::Str(Ch5,TEXT("stage"))==TEXT("road")&&S47J::Int(R,TEXT("v"))==1&&S47J::Bool(R,TEXT("routeReadback"))&&S47J::Num(Case,TEXT("clock"))==19200,
    TEXT("actual passed SC12 sibling departure checkpoint, full05:20 state; no fabricated route facts")))return;
   M()->Flow->Saves=FS47SaveStore(Root/TEXT("case1"));M()->Flow->Saves.Init();FS47SaveCard Card;Card.Clock=TEXT("05:20");Card.Place=TEXT("Mesa Diner");Card.Chapter=TEXT("All Night");FString Error;
   SaveMeta=M()->Flow->Saves.Save(1,TEXT("manual"),0,Departure,Card,Error);FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();
   if(!Check(SaveMeta&&Same(Reader.Load(SaveMeta->Id),Departure),TEXT("fresh reader equals complete SC12 departure on isolated disk: ")+Error))return;
   M()->Flow->ToTitle();if(!Check(M()->Mode==US47Main::EMode::Title,TEXT("ordinary title reached before departure Continue")))return;
   M()->Flow->OpenSave(*SaveMeta);++RestoreCount;Next(1);return;
  }
  if(!Armed||M()->Flow->bStarting||M()->World->bBusy)return;
  if(Stage==1)
  {
   const auto Case=S47J::Obj(Departure,TEXT("case"));
   if(!Check(C5()->bActive&&!C()->bActive&&M()->World->Area==TEXT("diner")&&Same(C5()->SaveState(),S47J::Obj(Case,TEXT("ch5")))&&FMath::Abs(M()->Story->Clock()-19200)<.2&&M()->Flow->GameSaveBlock().IsEmpty(),TEXT("normal disk/title/Continue preserves every SC12 source/receipt/map fact at safe05:20 departure")))return;
   if(!Snapshot(TEXT("departure-after-continue")))return;PathToTruck();Next(2);return;
  }
  if(Stage==2)
  {
   if(Now-Since>75){Result(false,TEXT("physical path from diner to service truck stalled"));return;}
   if(!Walk(Dt))return;
   const auto* It=M()->Inter->Get(TEXT("dinerTruck"));if(!Check(It&&bool(It->Use),TEXT("original service-truck interaction remains bound")))return;
   const FVector D=It->Object.WorldPosition()-S47Three::ToThree(M()->Player->EyeUE());M()->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M()->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Next(3);return;
  }
  if(Stage==3)
  {
   if(Now-Since<.5)return;
   const FString Pick=M()->Inter->Update(M()->Player->EyeUE(),M()->Player->ForwardUE());
   if(!Check(Pick==TEXT("dinerTruck")&&M()->Player->FocusedId()==Pick&&bool(M()->Player->OnUse),TEXT("actual settled player-camera ray/focus -> original service truck before ordinary OnUse")))return;
   M()->Player->OnUse(Pick);Next(4);return;
  }
  if(Stage==4)
  {
   if(!C()->bActive||!M()->World->bDriving||M()->World->Area!=TEXT("roswell")){if(Now-Since>15)Result(false,TEXT("normal Chapter6 drive did not begin"));return;}
   const auto* Cab=M()->World->CabTruck();
   if(!Check(Cab&&!Cab->IsPersonalVehicle()&&!M()->World->bCommuteVehicle,TEXT("actual active cab is SARO 07, never the personal commute car")))return;
   if(!Check((Mode==TEXT("legacy"))!=C()->RevisedFinale(),TEXT("revised route requires complete saved SC12 facts; legacy has no new art or requirements")))return;
   if(Mode==TEXT("line")){C()->JumpAt=TEXT("line");C()->PlaceJump();LinePlaced=true;}
   M()->World->Autopilot=24.6;LastCar=M()->World->OldDrive()->Pos;HaveCar=true;MaxMile=M()->World->OldRoad()->Where(LastCar.X,LastCar.Z).Mi;
   if(!Check(Mode!=TEXT("full")||MaxMile<.10,TEXT("full route actually starts on the diner lot, not a last-mile placement")))return;
   if(!Snapshot(TEXT("drive-start")))return;Next(5);return;
  }
  if((Stage==5||Stage==6)&&M()->Mode==US47Main::EMode::End)
  {
   EndAt=Now;if(!Check(C()->Stage==TEXT("end")&&C()->Pt>=34&&C()->Pt<34.4&&PullAt>EventAt,TEXT("original34second pullout reaches normal Flow finale")))return;
   if(!Snapshot(TEXT("normal-ending-start")))return;Next(7);return;
  }
  if(Stage==5||Stage==6||Stage==90)
  {
   auto* D=M()->World->OldDrive();auto* R=M()->World->OldRoad();if(!D||!R){Result(false,TEXT("actual old-road controller disappeared"));return;}
   const auto* Cab=M()->World->CabTruck();
   if(!Check(Cab&&!Cab->IsPersonalVehicle()&&!M()->World->bCommuteVehicle,TEXT("actual service cab remains selected throughout ordinary drive/event/pullout")))return;
   const double Mile=R->Where(D->Pos.X,D->Pos.Z).Mi;
   if(Stage!=90&&HaveCar){const double Moved=FVector2D(D->Pos.X-LastCar.X,D->Pos.Z-LastCar.Z).Size();if(!Check(Moved<FMath::Max(.8,Dt*42+.25),TEXT("continuous actual old-road physics; no mid-route placement")))return;DriveMetres+=Moved;LastCar=D->Pos;MaxMile=FMath::Max(MaxMile,Mile);}
   if(!Check(Mile<8.30,TEXT("10.4 is never reached; the motor interruption remains at C8.15")))return;
   if(C()->Stage==TEXT("event")||C()->Stage==TEXT("pullout"))
   {
    auto Sample=S47J::New();S47J::Set(Sample,TEXT("event"),C()->Et);S47J::Set(Sample,TEXT("pull"),C()->Pt);S47J::Set(Sample,TEXT("paused"),M()->bPausedByMenu);S47J::Set(Sample,TEXT("wallMs"),WallDt*1000);S47J::Set(Sample,TEXT("gameMs"),FPlatformTime::ToMilliseconds(GGameThreadTime));S47J::Set(Sample,TEXT("drawMs"),FPlatformTime::ToMilliseconds(GRenderThreadTime));S47J::Set(Sample,TEXT("gpuMs"),FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));Samples.Add(MakeShared<FJsonValueObject>(Sample));
   }
   if(Stage==90)
   {
    if(Now-Since<1.2)return;
    if(!Check(M()->bPausedByMenu&&Same(PauseBefore,C()->FinaleReadback()),TEXT("ordinary pause freezes exact event clock, cloud phase, actual components and material parameters")))return;
    M()->UI->Close();if(!Check(!M()->bPausedByMenu,TEXT("ordinary pause modal close resumes through its production callback")))return;DidPause=true;LastCar=D->Pos;Next(6);return;
   }
   if(C()->Stage==TEXT("drive"))
   {
    if(Mile>=7.65&&!Shots.Contains(TEXT("road-cloud-buildup"))){Picture(TEXT("road-cloud-buildup"));return;}
    if(!PendingPhoto.IsEmpty()){Picture(PendingPhoto);return;}
    return;
   }
   if(!EventAt)
   {
    EventAt=Now;
    if(!Check(C()->Stage==TEXT("event")&&C()->Et<.3&&FMath::Abs(M()->Story->Clock()-19740)<.3&&Mile>8.145&&Mile<8.16&&(Mode!=TEXT("full")||DriveMetres>13000),TEXT("ordinary front crossing at C8.15 begins actual05:29 EVENT after complete drive")))return;
    if(!Snapshot(TEXT("event-start")))return;
   }
   if(Mode!=TEXT("legacy"))
   {
    const auto Sky=C()->FinaleSky?C()->FinaleSky->Readback():FJ();
    if(!Check(S47J::Bool(Sky,TEXT("ready"))&&S47J::Bool(Sky,TEXT("shown"))&&S47J::Num(Sky,TEXT("craftOpacity"))==1&&S47J::Arr(Sky,TEXT("actualBankParameters")).Num()==2,TEXT("actual fresh-bound opaque craft and both depth-clipped finite banks are active")))return;
    for(const auto& V:S47J::Arr(Sky,TEXT("actualBankParameters")))if(!Check(S47J::Bool(V->AsObject(),TEXT("complete"))&&S47J::Num(V->AsObject(),TEXT("Samples"))==S47J::Num(Sky,TEXT("samples")),TEXT("actual own bank material parameters read back on selected Low/High policy")))return;
   }
   else if(!Check(!C()->FinaleSky,TEXT("legacy ending never constructs new sky art")))return;
   if(C()->Stage==TEXT("event"))
   {
    if(C()->Et>.3&&!Check(!D->bEngine&&!M()->Flow->GameSaveBlock().IsEmpty(),TEXT("engine actually dies without player request and road remains unsaveable")))return;
    if(!DidKey&&C()->Et>=2){if(!Check(C()->TurnKey()&&C()->Tries.Key==1,TEXT("original key attempt still works and cannot restart the engine")))return;DidKey=true;}
    if(!DidDoor&&C()->Et>=3){if(!Check(C()->TryDoor()&&C()->Tries.Door==1,TEXT("original door attempt remains offered but cannot open escape")))return;DidDoor=true;}
    if(C()->Et>=10&&!DidPause){PauseBefore=C()->FinaleReadback();M()->Flow->OpenPause();Next(90);return;}
    const double Times[]={4,14,23,43};const TCHAR* Names[]={TEXT("event-04-cloud-cover"),TEXT("event-14-reveal"),TEXT("event-23-craft"),TEXT("event-43-silence")};
    for(int32 I=0;I<4;++I)if(C()->Et>=Times[I]&&!Shots.Contains(Names[I])){Picture(Names[I]);return;}
    if(!PendingPhoto.IsEmpty())Picture(PendingPhoto);
    Next(6);return;
   }
   if(C()->Stage==TEXT("pullout"))
   {
    if(!PullAt){PullAt=Now;if(!Check(C()->Et>=47&&C()->Et<47.3&&C()->Pt<.3&&DidKey&&DidDoor&&DidPause,TEXT("all original event actions and47seconds complete before unchanged camera pullout"))||!Snapshot(TEXT("pullout-start")))return;}
    if(C()->Pt>=6&&!Shots.Contains(TEXT("pull-06-empty-seat"))){Picture(TEXT("pull-06-empty-seat"));return;}
    if(C()->Pt>=29&&!Shots.Contains(TEXT("pull-29-sky"))){Picture(TEXT("pull-29-sky"));return;}
    if(!PendingPhoto.IsEmpty())Picture(PendingPhoto);return;
   }
  }
  if(Stage==7)
  {
   const double E=Now-EndAt;
   if(E>=3&&!Shots.Contains(TEXT("original-ending-title"))){Picture(TEXT("original-ending-title"));return;}
   if(E>=84&&!Shots.Contains(TEXT("original-1947-photo"))){Picture(TEXT("original-1947-photo"));return;}
   if(!PendingPhoto.IsEmpty()){Picture(PendingPhoto);return;}
   if(M()->Mode!=US47Main::EMode::Title){if(E>115)Result(false,TEXT("unmodified title/credits/photo chain did not naturally return to title"));return;}
   if(!Check(PhotoBefore==PhotoHash()&&Shots.Num()>=(Mode==TEXT("legacy")?8:9),TEXT("original1947 image bytes unchanged and requested actual viewport phases completed")))return;
   if(Mode==TEXT("legacy")){Result(true,TEXT("legacy fixed ending remains unchanged, no revised sky or extra route gates"));return;}
   FS47SaveStore Reader(M()->Flow->Saves.Dir);Reader.Init();if(!Check(SaveMeta&&Same(Reader.Load(SaveMeta->Id),Departure),TEXT("whole ending never overwrote the safe diner departure checkpoint")))return;
   M()->Flow->OpenSave(*SaveMeta);++RestoreCount;Next(8);return;
  }
  if(Stage==8)
  {
   const auto Case=S47J::Obj(Departure,TEXT("case"));
   if(!Check(C5()->bActive&&!C()->bActive&&!C()->FinaleSky&&M()->World->Area==TEXT("diner")&&Same(C5()->SaveState(),S47J::Obj(Case,TEXT("ch5")))&&FMath::Abs(M()->Story->Clock()-19200)<.3,TEXT("ordinary post-ending Continue returns to exact source/receipt/P13/courier departure, with no duplicate finale objects")))return;
   if(!Snapshot(TEXT("post-ending-continue")))return;Result(true,TEXT("physical road to involuntary C8.15 stop, original47+34 ending, own sky phases and normal full disk/title/Continue"));
  }
 }
};

void US47Chapter6::ControlSC13Test(double Dt)
{
 if(!SC13Contract)SC13Contract=MakeShared<FS47SC13Contract>(M.Get());const auto Keep=SC13Contract;Keep->Tick(Dt);
}
void S47SC13Contract::Step(US47Main* Main,double Dt)
{
 if(Main&&Main->Story)if(auto* C=Cast<US47Chapter6>(Main->Story->Chapter(TEXT("ch6"))))C->ControlSC13Test(Dt);
}
