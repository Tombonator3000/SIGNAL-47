#include "S47FreeformJourneyTest.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Three.h"
#include "S47Startup.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

bool FS47FreeformJourneyTest::Check(bool Good,const FString& Text)
{
 ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Text);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),Stage);Evidence.Add(MakeShared<FJsonValueObject>(O));
 UE_LOG(LogTemp,Display,TEXT("S47 Journey %s stage%d: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Stage,*Text);
 if(!Good)Finish(false,Text);return Good;
}
void FS47FreeformJourneyTest::Go(int32 Next){Stage=Next;At=Age;}
void FS47FreeformJourneyTest::PlaceAt(const FString& Id,double Offset)
{
 M->UI->Close(true);const FVector P=M->Freeform->Field->InteractionPoint(Id);
 M->Player->Place(P.X-Offset,P.Z,0);
}
bool FS47FreeformJourneyTest::Restore(const FString& Name,bool bLegacy)
{
 auto State=M->Flow->GameState();
 if(bLegacy)
 {
  auto F=S47J::Obj(State,TEXT("freeform"));
  if(!Check(bool(F),TEXT("legacy fixture retains ordinary Freeform state")))return false;
  for(const TCHAR* K:{TEXT("serviceGuide"),TEXT("guideServiceBaseline"),TEXT("guideSupplyBaseline"),TEXT("guideSupply"),TEXT("guideFocus")})F->RemoveField(K);
 }
 const FString File=Root/(Name+TEXT(".json"));FString Text;
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(State),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("isolated full save written and freshly read: ")+Name))return false;
 State=S47J::Read(Text);if(!Check(bool(State),TEXT("full saved JSON parses: ")+Name))return false;
 US47Flow::FStart S;S.bDev=true;S.State=State;M->Flow->StartGame(S);++Restores;
 return Check(M->IsFreeform()&&M->Freeform->Active()&&M->Flow->CaseId==0,TEXT("normal Flow restores isolated Freeform: ")+Name);
}
void FS47FreeformJourneyTest::Finish(bool Good,const FString& Why)
{
 if(bDone)return;bDone=true;if(M){M->bPausedByMenu=false;if(M->Player)M->Player->TestMove.Reset();}
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("flowRestores"),Restores);O->SetArrayField(TEXT("evidence"),Evidence);
 S47J::Set(O,TEXT("method"),TEXT("Production service/supply/carry callbacks, real Walker movement, physical dropped parcel and actual S-03 encoder test. Explicit world/vehicle/player placement fixtures replace the long-distance drive and workstation approach. No service facts or completion counters are injected."));
 S47J::Set(O,TEXT("limits"),TEXT("Not a natural road journey, human input/route-finding acceptance, visual polish review or full regression. Isolated developer case 0 only; timeout 300 seconds."));
 if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
void FS47FreeformJourneyTest::Step(double Dt)
{
 if(bDone)return;
 if(!bInit)
 {
  bInit=true;if(!FParse::Param(FCommandLine::Get(),TEXT("S47FreeformJourneyTest"))){bDone=true;return;}
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/"))){Finish(false,TEXT("explicit temporary profile required"));return;}
  Root=FPaths::ProjectSavedDir()/TEXT("FreeformJourneyTest");IFileManager::Get().MakeDirectory(*Root,true);
 }
 Age+=FMath::Clamp(Dt,0.,.2);if(Age>300){Finish(false,TEXT("bounded journey timeout"));return;}
 if(!M||!M->Flow||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->World||!M->Player||M->Flow->bStarting||S47Startup::Blocking())return;
 auto& F=*M->Freeform;if(!F.Field)return;auto& W=*F.Field;
 if(Stage==0)
 {
  if(Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->IsFreeform(),TEXT("temporary developer case 0 required")))return;
  F.Director->Stop();PlaceAt(TEXT("ff:bench"));if(!Check(!F.StartServiceGuide(true),TEXT("service order cannot be accepted remotely")))return;
  const auto* Ops=M->Web->Item(TEXT("opsTerminal"));if(!Check(Ops!=nullptr,TEXT("actual OPS terminal exists")))return;
  const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X,P.Z-.8,0);
  F.Use(TEXT("opsTerminal"));if(!Check(F.StartServiceGuide(true),TEXT("optional supply route accepted at physical OPS")))return;M->UI->Close(true);
  ServiceBefore=W.ServiceCompletions();DeliveryBefore=W.ServiceSupplyDeliveries();
  if(!Check(!F.CompleteServiceGuide()&&W.ServiceStage()==1&&!F.ServiceObjective().IsEmpty(),TEXT("unperformed service cannot be archived; diagnosis is the next action")))return;
  F.OpenServiceGuide();FScreenshotRequest::RequestScreenshot(Root/TEXT("service-guide.png"),true,false);Go(100);return;
 }
 if(Stage==100)
 {
  if(Age-At<.5)return;M->UI->Close(true);
  F.Use(TEXT("crtLeft"));W.DiagnoseService();M->UI->Close(true);
  if(!Check(W.ServiceStage()==2,TEXT("status-terminal diagnosis precedes field work")))return;
  M->World->Enter(TEXT("road"));Go(1);return;
 }
 if(Stage==1)
 {
  if(M->World->bBusy||Age-At<.5)return;
  M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));PlaceAt(TEXT("ff:shop"));Go(2);return;
 }
 if(Stage==2)
 {
  if(M->World->bBusy||Age-At<.5)return;
  if(!Check(W.Purchase(TEXT("donor"))&&W.ParcelStage()==1&&F.Budget()==105,TEXT("real donor purchase charges once and leaves a physical parcel")))return;
  W.Use(TEXT("ff:parcel"));if(!Check(W.ParcelStage()==2,TEXT("parcel picked up before transport")))return;
  Before=M->Player->Pos;M->Player->TestMove=FVector2D(0,-1);Go(3);return;
 }
 if(Stage==3)
 {
  if(Age-At<.6)return;M->Player->TestMove.Reset();
  if(!Check(FVector2D(M->Player->Pos.X-Before.X,M->Player->Pos.Z-Before.Z).Size()>.15,TEXT("actual Walker advances while carrying parcel")))return;
  if(!Check(W.DropParcel(true)&&W.ParcelStage()==4,TEXT("drop transfers carried parcel to physical world")))return;Go(4);return;
 }
 if(Stage==4)
 {
  if(Age-At<2)return;
  if(!Check(W.ParcelSimulating()&&W.ParcelImpactCount()>0,TEXT("dropped parcel uses native physics and reports a real physical contact")))return;
  Before=W.ParcelPosition();
  const FVector D=Before-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  FScreenshotRequest::RequestScreenshot(Root/TEXT("physical-parcel.png"),true,false);
  M->bPausedByMenu=true;Go(5);return;
 }
 if(Stage==5)
 {
  if(Age-At<1)return;
  if(!Check(W.ParcelPosition().Equals(Before,.05),TEXT("pause holds dropped parcel position")))return;
  M->bPausedByMenu=false;if(!Restore(TEXT("dropped-parcel")))return;Go(6);return;
 }
 if(Stage==6)
 {
  if(Age-At<.5)return;F.Director->Stop();
  if(!Check(W.ParcelStage()==4&&W.ParcelPosition().Equals(Before,.15),TEXT("dropped parcel pose survives disk/Flow restore")))return;
  PlaceAt(TEXT("ff:parcel"));W.Use(TEXT("ff:parcel"));if(!Check(W.ParcelStage()==2,TEXT("physical dropped parcel can be retrieved")))return;
  PlaceAt(TEXT("ff:cargo"));W.Use(TEXT("ff:cargo"));if(!Check(W.ParcelStage()==3,TEXT("retrieved parcel secured in vehicle cargo")))return;
  if(!Restore(TEXT("loaded-parcel")))return;Go(7);return;
 }
 if(Stage==7)
 {
  if(Age-At<.5)return;F.Director->Stop();if(!Check(W.ParcelStage()==3,TEXT("loaded cargo survives disk/Flow restore")))return;
  M->World->Enter(TEXT("saro"));Go(8);return;
 }
 if(Stage==8)
 {
  if(M->World->bBusy||Age-At<.5)return;M->World->PlaceTruck(TEXT("saro"));PlaceAt(TEXT("ff:cargo"));W.Use(TEXT("ff:cargo"));
  if(!Check(W.ParcelStage()==2,TEXT("cargo unloaded before depot delivery")))return;
  PlaceAt(TEXT("ff:bench"));W.Use(TEXT("ff:bench"));
  if(!Check(W.ParcelStage()==0&&W.InventoryCount(TEXT("donor"))==1&&W.ServiceSupplyDeliveries()==DeliveryBefore+1,TEXT("transported donor delivered exactly once to SARO")))return;
  W.Use(TEXT("ff:bench"));M->UI->Close(true);
  if(!Check(W.ServiceSupplyDeliveries()==DeliveryBefore+1,TEXT("reopening depot does not duplicate supply delivery")))return;
  if(!Check(W.StartDonorTest(),TEXT("physical bench starts donor test from delivered stock")))return;Go(9);return;
 }
 if(Stage==9)
 {
  if(Age-At<3.5||W.BenchBusy())return;
  if(!Check(W.InventoryCount(TEXT("donor"))==0&&W.InventoryCount(TEXT("relay"))==2&&F.HasFinding(TEXT("donor-relay")),TEXT("actual donor bench test produces one compatible relay")))return;
  PlaceAt(TEXT("ff:service"));W.Use(TEXT("ff:service"));W.Use(TEXT("ff:service"));W.Use(TEXT("ff:service"));W.Use(TEXT("ff:service"));
  if(!Check(W.ServiceStage()==6&&W.InventoryCount(TEXT("relay"))==1,TEXT("local isolation, cover and relay exchange consume one real spare")))return;
  int32 I=0;for(;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==TEXT("S-03"))break;
  if(!Check(I<M->Web->DishCount(),TEXT("S-03 actual encoder found")))return;InitialAz=M->Web->DishAz(I);
  W.Use(TEXT("ff:service"));if(!Check(W.ServiceStage()==7&&!F.CompleteServiceGuide(),TEXT("actual movement starts; unfinished service cannot be archived")))return;Go(10);return;
 }
 if(Stage==10)
 {
  for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==TEXT("S-03"))bSawOutward|=FMath::Abs(FMath::FindDeltaAngleDegrees(InitialAz,M->Web->DishAz(I)))>4.;
  if(W.ServiceStage()!=9)return;
  if(!Check(bSawOutward&&W.ServiceCompletions()==ServiceBefore+1&&F.HasFinding(TEXT("service-s03")),TEXT("real S-03 outward and return movement retains service evidence")))return;
  const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X,P.Z-.8,0);
  if(!Check(F.CompleteServiceGuide()&&F.HasFinding(TEXT("service-work-order")),TEXT("physical OPS verifies and archives actual service")))return;
  const int32 Count=F.FindingCount();if(!Check(!F.CompleteServiceGuide()&&F.FindingCount()==Count&&F.Budget()==105,TEXT("repeat archive cannot duplicate record or payment")))return;
  if(!Restore(TEXT("legacy-v1-without-guide"),true))return;Go(11);return;
 }
 if(Stage==11)
 {
  if(Age-At<.5)return;F.Director->Stop();
  if(!Check(F.ServiceObjective().IsEmpty()&&!F.CompleteServiceGuide()&&F.Budget()==105&&F.HasFinding(TEXT("service-work-order")),TEXT("older v1 without guide fields loads safely and retains original archive")))return;
  Finish(true);
 }
}
