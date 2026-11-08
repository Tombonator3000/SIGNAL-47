#include "S47HighwayUfoTest.h"
#include "S47HighwayUfo.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "Kismet/GameplayStatics.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47RoadTerrain.h"
#include "S47Startup.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"

FS47HighwayUfoTest::FS47HighwayUfoTest(US47Main* Main):M(Main)
{
 FString Kind;bEnabled=FParse::Value(FCommandLine::Get(),TEXT("S47HighwayUfoTest="),Kind);bFollow=Kind==TEXT("follow");bKeepDriving=!bFollow&&FParse::Param(FCommandLine::Get(),TEXT("S47HighwayKeepDriving"));
 bNorth=FParse::Param(FCommandLine::Get(),TEXT("S47HighwayNorth"));
 if(bEnabled&&Kind!=TEXT("chase")&&Kind!=TEXT("follow")){bEnabled=false;UE_LOG(LogTemp,Error,TEXT("S47: highway test expects chase or follow"));}
}
FS47HighwayUfoTest::~FS47HighwayUfoTest()=default;
bool FS47HighwayUfoTest::Check(bool Good,const FString& Text)
{
 ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("passed"),Good);S47J::Set(O,TEXT("check"),Text);Evidence.Add(MakeShared<FJsonValueObject>(O));
 if(Good){UE_LOG(LogTemp,Display,TEXT("S47: highway UFO test PASS: %s"),*Text);}
 else {UE_LOG(LogTemp,Error,TEXT("S47: highway UFO test FAIL: %s"),*Text);}
 if(!Good)Finish(false,Text);return Good;
}
void FS47HighwayUfoTest::Shot(const TCHAR* Name)
{
 FScreenshotRequest::RequestScreenshot(Root/(FString(Name)+TEXT(".png")),true,false);
 if(M&&M->HighwayUfo)
 {
  auto R=M->HighwayUfo->Readback();S47J::Set(R,TEXT("testKeepDriving"),bKeepDriving);
  if(const auto* D=M->World?M->World->CabDrive():nullptr)
  {S47J::Set(R,TEXT("testPlayerPosition"),TArray<double>{D->Pos.X,D->Pos.Y,D->Pos.Z});S47J::Set(R,TEXT("testPlayerSpeed"),D->Speed);S47J::Set(R,TEXT("testSourceDistance"),FVector::Dist(D->Pos,M->HighwayUfo->UfoPosition()));}
  FFileHelper::SaveStringToFile(S47J::Write(R,true),*(Root/(FString(Name)+TEXT(".json"))));
 }
}
void FS47HighwayUfoTest::Finish(bool Good,const FString& Why)
{
 if(bDone)return;bDone=true;
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("diskRestores"),Restores);S47J::Set(O,TEXT("variant"),bFollow?TEXT("follow"):TEXT("chase"));S47J::Set(O,TEXT("bound"),bNorth?TEXT("north"):TEXT("south"));S47J::Set(O,TEXT("playerMetres"),Travel);S47J::Set(O,TEXT("keepDriving"),bKeepDriving);O->SetArrayField(TEXT("evidence"),Evidence);
 S47J::Set(O,TEXT("method"),TEXT("One explicit road start and nighttime clock fixture in developer Freeform case 0. Existing World StartTrip/vehicle input performs the drive; chase uses the natural trigger, follow uses explicit Force. Actual presentation/collision readback, active pause, fresh disk read of Flow GameState and subsystem Begin restore, then bounded departure. No player relocation after the initial fixture."));
 S47J::Set(O,TEXT("limits"),TEXT("Automated native input, not human play acceptance. Subsystem restore uses the actual saved Flow field but does not claim a full title/Continue journey. Follow probes outrunning; physical building/terrain concealment remains a separate route to inspect."));
 if(M&&M->HighwayUfo)S47J::Set(O,TEXT("final"),M->HighwayUfo->Readback());
 if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 if(M){if(M->bPausedByMenu&&M->UI)M->UI->Close();if(M->World){M->World->Autopilot.Reset();M->World->TestInput=nullptr;}}
 FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
bool FS47HighwayUfoTest::Persist()
{
 const auto Before=M->HighwayUfo->Save();const FVector Ufo=M->HighwayUfo->UfoPosition();
 FS47SaveStore Writer(Root/TEXT("disk"));Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("Highway UFO native probe");Card.Place=TEXT("road");Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;
 FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,M->Flow->GameState(),Card,Error);
 if(!Check(Meta.IsSet(),TEXT("isolated Flow state writes to disk: ")+Error))return false;
 FS47SaveStore Reader(Root/TEXT("disk"));Reader.Init();const auto State=Reader.Load(Meta->Id);const auto Saved=S47J::Obj(State,TEXT("highwayUfo"));
 if(!Check(Saved.IsValid()&&S47J::Str(Saved,TEXT("outcome"))==S47J::Str(Before,TEXT("outcome")),TEXT("fresh disk reader finds the integrated highwayUfo field")))return false;
 M->HighwayUfo->Begin(Saved);++Restores;
 if(!Check(FMath::IsNearlyEqual(M->HighwayUfo->EventAge(),S47J::Num(Before,TEXT("age")),1e-6)&&M->HighwayUfo->UfoPosition().Equals(Ufo,.001)&&S47J::Int(M->HighwayUfo->Save(),TEXT("serial"))==S47J::Int(Before,TEXT("serial")),TEXT("restore preserves phase time, world transform and incident identity")))return false;
 // Presentation is recreated on the next production Step, before this check.
 return true;
}
void FS47HighwayUfoTest::Step(double Dt)
{
 if(!bEnabled||bDone)return;
 const double Now=FPlatformTime::Seconds();
 if(Started==0)
 {
  Started=Now;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/"))){Finish(false,TEXT("explicit temporary UserDir required"));return;}
  Root=FPaths::ProjectSavedDir()/TEXT("HighwayUfoTest");IFileManager::Get().MakeDirectory(*Root,true);
 }
 if(Now-Started>210){Finish(false,TEXT("bounded native road probe timed out"));return;}
 if(!M||!M->Flow||!M->Freeform||!M->Freeform->Active()||!M->HighwayUfo||!M->World||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Story||M->Flow->bStarting||S47Startup::Blocking())return;
 auto& H=*M->HighwayUfo;
 if(Stage==0)
 {
  if(Now-Started<3)return;
  if(!Check(M->IsFreeform()&&M->Flow->bDevRun&&M->Flow->CaseId==0,TEXT("isolated Freeform developer case 0")))return;
  if(M->Freeform->Director)M->Freeform->Director->Stop();
  // The natural trigger waits for their hours in Freeform (TheirHours): three in the morning, not the evening it once was.
  M->Freeform->TestAdvance(FMath::Fmod(3*3600.-FMath::Fmod(M->Freeform->Clock(),86400.)+86400.,86400.));
  H.Begin(nullptr);
  // Its own lane either way, a little before the stretch where the encounter starts by itself.
  const double StartX=bNorth?1.8:-1.8,StartZ=bNorth?1700.:620.;
  FS47TripOpts O;O.AtPos=FVector(8000+StartX,S47RoadTerrain::SurfaceY(StartX,StartZ),StartZ);O.AtHeading=bNorth?0.:UE_DOUBLE_PI;O.Leg=TEXT("road");
  M->World->StartTrip(TEXT("road"),TEXT("diner"),O);
  M->World->TestInput=[this]{FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;if(D)I.Throttle=bFollow?1.:(D->Speed<14?1:0);return I;};
  Stage=1;At=Now;return;
 }
 if(!M->World->CabDrive()){if(Now-At>12)Finish(false,TEXT("native road cab failed to start or was lost"));return;}
 const auto D=M->World->CabDrive();
 if(Stage!=3&&LastZ!=0)Travel+=FMath::Abs(D->Pos.Z-LastZ);LastZ=D->Pos.Z;
 if(Stage==1)
 {
  if(!M->World->bDriving||M->World->bBusy||D->Speed<=1)return;
  PlayerStartZ=D->Pos.Z;
  if(bFollow&&!Check(H.Force(true),TEXT("follow variant starts through its production request")))return;
  Stage=2;At=Now;return;
 }
 if(Stage==2)
 {
  if(H.EventAge()<6)return;
  const auto R=H.Readback();
  if(!Check(H.Busy()&&S47J::Bool(S47J::Obj(R,TEXT("presentation")),TEXT("presented")),TEXT("native road UFO is present after ordinary drive trigger")))return;
  if(!Check(bFollow||S47J::Bool(R,TEXT("carShown")),TEXT("patrol car is visible in the pursuit variant")))return;
  // Along the road as it is driven: the light and the patrol are ahead of the driver when it starts, and go on the same way.
  const double Way=bNorth?-1.:1.;
  UE_LOG(LogTemp,Display,TEXT("S47: highway UFO test bound %s: player at %.0f, patrol at %.0f, light at %.0f (road metres); patrol x %.1f"),bNorth?TEXT("north"):TEXT("south"),D->Pos.Z,H.CarPosition().Z,H.UfoPosition().Z,H.CarPosition().X-8000);
  if(!Check(FMath::IsNearlyEqual(S47J::Num(R,TEXT("direction")),Way)&&Way*(D->Pos.Z-PlayerStartZ)>0,FString::Printf(TEXT("encounter runs the way the player drives, %s"),bNorth?TEXT("north"):TEXT("south"))))return;
  if(!Check(bFollow||(Way*(H.UfoPosition().Z-H.CarPosition().Z)>0&&Way*(H.CarPosition().Z-D->Pos.Z)>0&&Way*(H.CarPosition().X-8000)<0),TEXT("the light leads the patrol, the patrol is ahead of the player in its own right-hand lane")))return;
  if(!Check(D->bEngine&&D->Held==0&&D->Speed>1,TEXT("encounter preserves live player engine, steering and propulsion")))return;
  Shot(TEXT("01-approach"));Stage=20;At=Now;return;
 }
 if(Stage==20)
 {
  // Allow the world screenshot to finish before opening the real pause UI.
  if(Now-At<.2)return;
  PausePlayer=D->Pos;PauseUfo=H.UfoPosition();PauseCar=H.CarPosition();PausedAge=H.EventAge();
  M->Flow->OpenPause();
  if(!Check(M->bPausedByMenu&&M->ModalOpen()&&UGameplayStatics::IsGamePaused(M->Web.Get()),TEXT("ordinary pause menu suspends the world and opens its modal")))return;
  Stage=3;At=Now;return;
 }
 if(Stage==3)
 {
  if(Now-At<.8)return;
  if(!Check(H.EventAge()==PausedAge&&H.UfoPosition().Equals(PauseUfo,.001)&&H.CarPosition().Equals(PauseCar,.001)&&D->Pos.Equals(PausePlayer,.001),FString::Printf(TEXT("pause holds encounter, patrol and driven vehicle in the same frame (age %.9fs, UFO %.6fm, patrol %.6fm, player %.6fm)"),H.EventAge()-PausedAge,FVector::Dist(H.UfoPosition(),PauseUfo),FVector::Dist(H.CarPosition(),PauseCar),FVector::Dist(D->Pos,PausePlayer))))return;
  M->UI->Close();
  if(!Check(!M->bPausedByMenu&&!M->ModalOpen()&&!UGameplayStatics::IsGamePaused(M->Web.Get()),TEXT("ordinary Resume releases pause and restores the driving frame")))return;
  bPauseChecked=true;Stage=4;At=Now;return;
 }
 if(Stage==4)
 {
  if(H.EventAge()<10)return;if(!Persist())return;bRestored=true;Stage=5;At=Now;return;
 }
 if(Stage==5)
 {
  if(Now-At<.5)return;
  if(!Check(!H.PhotoIncidentId().IsEmpty()&&!H.PhotoParts().IsEmpty(),TEXT("restored live craft exposes actual meshes and stable photo incident")))return;
  if(!bFollow&&!bKeepDriving)
  {StoppedCarZ=H.CarPosition().Z;M->World->TestInput=[this]{FS47DriveIn I;const auto* D=M&&M->World?M->World->CabDrive():nullptr;I.Throttle=D&&D->Speed>.05?-1:0;return I;};}
  Stage=6;At=Now;return;
 }
 if(Stage==6)
 {
  if(!bFollow&&!bStopChecked&&H.EventAge()>20)
  {
   if(bKeepDriving)
   {if(!Check(D->Speed>1&&D->bEngine&&D->Held==0,TEXT("optional kept-driving route retains ordinary propulsion through the pursuit")))return;}
   else if(!Check(FMath::Abs(D->Speed)<.2&&(bNorth?StoppedCarZ-H.CarPosition().Z:H.CarPosition().Z-StoppedCarZ)>50,TEXT("player can stop while patrol and UFO continue on their own route")))return;
   bStopChecked=true;
  }
  const FVector ToUfo=H.UfoPosition()-D->Pos;
  const bool CloseAhead=ToUfo.Size()<=60&&FVector::DotProduct(ToUfo,FVector(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading)))>0;
  const bool PassFrame=bKeepDriving?H.EventAge()>=24&&(CloseAhead||H.EventAge()>=30.5):H.EventAge()>=27;
  if(!bPassShot&&PassFrame&&H.EventAge()<32){Shot(TEXT("02-near-pass"));bPassShot=true;}
  if(H.StageName()!=TEXT("done"))return;
  const auto R=H.Readback();
  if(!Check(!S47J::Bool(R,TEXT("active"))&&S47J::Num(R,TEXT("rest"))>0&&D->bEngine&&D->Held==0,TEXT("departure completes and reserves quiet time without capture or engine loss")))return;
  if(!Check(!S47J::Bool(S47J::Obj(R,TEXT("presentation")),TEXT("presented")),TEXT("departure removes all UFO presentation")))return;
  if(bFollow&&!Check(S47J::Bool(R,TEXT("escaped")),TEXT("maximum ordinary driving speed can genuinely outrun the bounded pursuit")))return;
  Shot(TEXT("03-after-departure"));Stage=7;At=Now;return;
 }
 if(Stage==7)
 {
  if(Now-At<.8)return;
  Finish(bPauseChecked&&bRestored&&bPassShot&&(bFollow||bStopChecked),TEXT("bounded native road encounter, pause, disk state and departure checked"));
 }
}
