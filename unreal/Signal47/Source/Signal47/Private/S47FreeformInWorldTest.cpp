#include "S47FreeformInWorldTest.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformAudioDesk.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Three.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Inter.h"
#include "S47Audio.h"
#include "S47Score.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 const int32 Screens[]={3,0,2,1};
 const TCHAR* Names[]={TEXT("operations"),TEXT("status"),TEXT("tracking"),TEXT("receiver")};
}
bool FS47FreeformInWorldTest::Check(bool Good,const FString& Text)
{
 ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Text);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),Stage);Evidence.Add(MakeShared<FJsonValueObject>(O));
 UE_LOG(LogTemp,Display,TEXT("S47 InWorld %s stage%d: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Stage,*Text);
 if(!Good)Finish(false,Text);return Good;
}
void FS47FreeformInWorldTest::Go(int32 Next){Stage=Next;At=Age;}
void FS47FreeformInWorldTest::AimAt(const FVector& Point)
{
 const FVector D=Point-S47Three::ToThree(M->Player->EyeUE());
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
bool FS47FreeformInWorldTest::UseAimedAudio(const FString& Id)
{
 if(!Check(M->Inter&&bool(M->Player->OnUse),TEXT("native interaction and Walker use callback exist")))return false;
 const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
 if(!Check(Picked==Id,TEXT("actual native eye ray picks physical tape key: ")+Id+TEXT("; picked ")+Picked))return false;
 M->Player->OnUse(Picked);return true;
}
bool FS47FreeformInWorldTest::OpenScreen(int32 Index)
{
 M->UI->Close(true);FVector C,U,A;FVector2D Size;
 if(!Check(M->Web->CrtGlass(Screens[Index],C,U,A,Size),FString(TEXT("actual CRT glass geometry exists: "))+Names[Index]))return false;
 FVector Normal=FVector::CrossProduct(A,U).GetSafeNormal();
 if(FVector::DotProduct(Normal,M->Player->EyeUE()-C)<0)Normal=-Normal;
 const FVector Approach=S47Three::ToThree(C+Normal*85.);M->Player->Place(Approach.X,Approach.Z,0);AimAt(S47Three::ToThree(C));
 auto& F=*M->Freeform;ClockBefore=F.Clock();
 if(Index==0)F.OpenOperations();else if(Index==1)F.OpenStatus();else if(Index==2)F.OpenTracking();else F.OpenReceiver();
 return Check(F.TerminalOnGlass()&&F.ActiveTerminalScreen()==Screens[Index],FString(TEXT("production panel opens on its physical CRT without fallback: "))+Names[Index]);
}
FString FS47FreeformInWorldTest::Cards(int32 Day)const
{
 FString Out;if(!M||!M->Freeform)return Out;
 // Stable, explicit source/receiver parameters, excluding changing work progress.
 for(const auto& J:M->Freeform->Jobs())if(J.Day==Day)
 {
  const auto& C=J.Card;const auto& P=C.Receiver;
  Out+=FString::Printf(TEXT("%s|%d|%d|%d|%d|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%.9f|%d|%s\n"),*J.Id,J.Day,J.Slot,J.Source,J.bGenerated?1:0,P.F,P.FTol,P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1],P.Az[0],P.Az[1],C.El,C.Seconds,C.SuggestedClock,int32(C.Control),*P.Card);
 }
 return Out;
}
void FS47FreeformInWorldTest::CaptureCards(const FString& Phase)
{
 auto O=S47J::New();S47J::Set(O,TEXT("phase"),Phase);S47J::Set(O,TEXT("day"),M->Freeform->Day());S47J::Set(O,TEXT("sourceParameterCards"),Cards(M->Freeform->Day()));Evidence.Add(MakeShared<FJsonValueObject>(O));
}
void FS47FreeformInWorldTest::ParkAtPump()
{
 M->UI->Close(true);const FVector P=M->Freeform->Field->InteractionPoint(TEXT("ff:pump"));
 // Explicit native parking fixture: no claim of a natural drive from SARO.
 M->Player->Place(P.X+2.5,P.Z,0);const double Floor=M->Player->FloorY;
 auto Pose=S47J::New();S47J::Set(Pose,TEXT("at"),FString(TEXT("diner")));S47J::Set(Pose,TEXT("x"),P.X+2.5);S47J::Set(Pose,TEXT("y"),Floor);S47J::Set(Pose,TEXT("z"),P.Z);S47J::Set(Pose,TEXT("h"),0.);S47J::Set(Pose,TEXT("p"),0.);S47J::Set(Pose,TEXT("r"),0.);
 M->World->PlaceTruck(TEXT("diner"),Pose);M->Player->Place(P.X+1.2,P.Z,0);AimAt(P);
}
bool FS47FreeformInWorldTest::Restore(const FString& Name)
{
 M->UI->Close(true);auto State=M->Flow->GameState();const FString File=Root/(Name+TEXT(".json"));FString Text;
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(State),*File)&&FFileHelper::LoadFileToString(Text,*File),TEXT("isolated full save written and freshly read: ")+Name))return false;
 State=S47J::Read(Text);if(!Check(bool(State),TEXT("saved JSON parses: ")+Name))return false;
 US47Flow::FStart Start;Start.bDev=true;Start.State=State;M->Flow->StartGame(Start);++Restores;
 return Check(M->IsFreeform()&&M->Freeform->Active()&&M->Flow->CaseId==0,TEXT("ordinary Flow restores temporary Freeform case 0: ")+Name);
}
void FS47FreeformInWorldTest::Finish(bool Good,const FString& Why)
{
 if(bDone)return;bDone=true;
 const bool ScreensOnly=FParse::Param(FCommandLine::Get(),TEXT("S47FreeformScreensOnly"));
 const bool SearchOnly=FParse::Param(FCommandLine::Get(),TEXT("S47ReceiverSearchOnly"));
 const bool SoundOnly=FParse::Param(FCommandLine::Get(),TEXT("S47SoundReferencesOnly"));
 if(Good)for(const TCHAR* Name:{TEXT("operations.png"),TEXT("status.png"),TEXT("tracking.png"),TEXT("receiver.png"),TEXT("pump-before.png"),TEXT("pump-after.png"),TEXT("actual-jeep-driver.png"),TEXT("fysisk-audio.png"),TEXT("space-reference-desk.png"),TEXT("receiver-search.png")})
 {
  if(SearchOnly!=(FString(Name)==TEXT("receiver-search.png")))continue;
  if(!SearchOnly&&SoundOnly!=(FString(Name)==TEXT("space-reference-desk.png")))continue;
  if(ScreensOnly && FString(Name)!=TEXT("operations.png") && FString(Name)!=TEXT("status.png") && FString(Name)!=TEXT("tracking.png") && FString(Name)!=TEXT("receiver.png"))continue;
  TArray<uint8> Bytes;const bool PNG=FFileHelper::LoadFileToArray(Bytes,*(Root/Name))&&Bytes.Num()>1000&&Bytes[0]==137&&Bytes[1]==80&&Bytes[2]==78&&Bytes[3]==71;
  if(!Check(PNG,FString(TEXT("native screenshot exists with PNG signature: "))+Name)){Good=false;}
 }
 const FString Reason=(!Good&&Why.IsEmpty())?FString(TEXT("required native screenshot was not captured")):Why;
 if(M){M->bPausedByMenu=false;if(M->UI)M->UI->Close(true);}
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Reason);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("flowRestores"),Restores);S47J::Set(O,TEXT("elapsedActiveDriverSeconds"),Age);O->SetArrayField(TEXT("evidence"),Evidence);
 S47J::Set(O,TEXT("scope"),ScreensOnly?TEXT("four physical screens and camera only"):TEXT("full in-world equipment journey"));
 S47J::Set(O,TEXT("method"),ScreensOnly?TEXT("Production four-screen opening, native camera/window, live clock and four screenshots. Rendering-only follow-up; no repeated pump or audio journey."):FString(TEXT("Production terminal and pump callbacks, actual CRT geometry/camera/window, clock, pause and disk/Flow restore. Explicit one-day queue fixture. Physical tape comparison uses real profile tuning, settled encoders, timed acquisition/control, actual native eye-ray Pick and Walker OnUse at the regular pump and on ORIGINAL/PLAY, timed original/control listening, mark and retain callbacks without a modal.")));
 S47J::Set(O,TEXT("limits"),TEXT("Isolated /tmp developer case 0 only. Explicit camera/player/vehicle/equipment placements replace natural navigation/driving. SetAnomaly(true) is an explicit developer fixture; no completed measurement, coverage, finding or reward is injected. Mounted Jeep mesh identities and a native driver-seat screenshot are checked, but windshield/mirror visual acceptance still requires review of that image. No natural drive, actual mouse navigation, human listening/readability acceptance, full-night or whole-game regression. Bounded below 240 seconds."));
 if(SoundOnly)
 {
  S47J::Set(O,TEXT("scope"),TEXT("six staged NASA/Nebula references at the physical SARO desk"));
  S47J::Set(O,TEXT("method"),TEXT("Explicit isolated player placement, native eye-ray Pick/OnUse for REF PLAY, production next/stop callbacks, no comparison coverage, leaving range stops playback, native screenshot."));
  S47J::Set(O,TEXT("limits"),TEXT("No human listening acceptance, natural navigation, full equipment journey or whole-game regression. Audio sample validity is checked separately by the offline runtime graph."));
 }
 if(SearchOnly)
 {
  S47J::Set(O,TEXT("scope"),TEXT("continuous search monitor on the physical Freeform receiver"));
  S47J::Set(O,TEXT("method"),TEXT("Production CRT opening and live session update, explicit receiver dial values, carrier graph response, music hold, screen switch, power loss and native screenshot."));
  S47J::Set(O,TEXT("limits"),TEXT("Isolated developer profile and explicit dial/camera fixtures; no actual mouse navigation or human listening acceptance. PCM is tested separately by ReceiverMonitorSelfTest."));
 }
 if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
void FS47FreeformInWorldTest::Step(double Dt)
{
 if(bDone)return;
 if(!bInit)
 {
  bInit=true;if(!FParse::Param(FCommandLine::Get(),TEXT("S47FreeformInWorldTest"))){bDone=true;return;}
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/"))){Finish(false,TEXT("explicit temporary profile required"));return;}
  Root=FPaths::ProjectSavedDir()/TEXT("FreeformInWorldTest");IFileManager::Get().MakeDirectory(*Root,true);
 }
 Age+=FMath::Clamp(Dt,0.,.2);if(Age>238){Finish(false,TEXT("bounded in-world test timeout"));return;}
 if(!M||!M->Flow||!M->Freeform||!M->Freeform->Active()||!M->Freeform->Field||!M->Web||!M->Web->IsLoaded()||!M->World||!M->Player||M->Flow->bStarting||S47Startup::Blocking())return;
 auto& F=*M->Freeform;auto& W=*F.Field;
 if(Stage==0)
 {
  if(Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->IsFreeform(),TEXT("temporary developer Freeform case 0 required")))return;
  if(F.Director)F.Director->Stop();DayBefore=F.Day();CardsBefore=Cards(DayBefore);CaptureCards(TEXT("before-save"));
  if(!Check(F.Jobs().Num()>=3&&!CardsBefore.IsEmpty(),TEXT("daily source parameter cards exist in production session")))return;
  if(FParse::Param(FCommandLine::Get(),TEXT("S47ReceiverSearchOnly")))
  {if(!OpenScreen(3))return;Go(50);return;}
  if(FParse::Param(FCommandLine::Get(),TEXT("S47SoundReferencesOnly")))
  {
   if(!Check(F.AudioDesk.IsValid(),TEXT("production reference desk exists")))return;
   const FVector P=F.AudioDesk->InteractionPoint(TEXT("ff:audio:ref_play"));
   M->Player->Place(P.X,P.Z-1.2,0);AimAt(P);ScreenIndex=0;Go(38);return;
  }
  if(!OpenScreen(0))return;Go(1);return;
 }
 if(Stage==1)
 {
  if(Age-At<.9)return;
  if(!Check(F.TerminalOnGlass()&&F.ActiveTerminalScreen()==Screens[ScreenIndex]&&F.TerminalWindowRendered(),FString(TEXT("actual virtual window rendered on native glass after camera transition: "))+Names[ScreenIndex]))return;
  if(!Check(F.Clock()>ClockBefore&&!F.Held(),FString(TEXT("active observing clock advances while physical terminal is open: "))+Names[ScreenIndex]))return;
  FScreenshotRequest::RequestScreenshot(Root/(FString(Names[ScreenIndex])+TEXT(".png")),true,false);Go(2);return;
 }
 if(Stage==2)
 {
  if(Age-At<.25)return;M->UI->Close(true);
  if(++ScreenIndex<4){if(!OpenScreen(ScreenIndex))return;Go(1);return;}
  if(FParse::Param(FCommandLine::Get(),TEXT("S47FreeformScreensOnly"))){Finish(true,TEXT("four native workstations rendered"));return;}
  if(!Restore(TEXT("cards-and-terminals")))return;Go(3);return;
 }
 if(Stage==3)
 {
  if(Age-At<.5)return;if(F.Director)F.Director->Stop();CaptureCards(TEXT("after-save"));
  if(!Check(Cards(DayBefore)==CardsBefore,TEXT("all original source/receiver card parameters survive disk and Flow")))return;
  if(!Check(!F.TerminalOnGlass()&&F.ActiveTerminalScreen()==-1&&!M->ModalOpen(),TEXT("restored scene has no orphan terminal or modal camera hold")))return;
  if(!Check(F.TestAdvance(86400.)&&F.Day()==DayBefore+1,TEXT("explicit one-day fixture generates the next production observing queue")))return;
  CaptureCards(TEXT("after-day-advance"));int32 Today=0;bool Valid=true;
  for(const auto& J:F.Jobs())if(J.Day==F.Day()){++Today;Valid&=J.bGenerated&&FS47FreeformJobs::Valid(J.Card);}
  if(!Check(Today==3&&Valid&&Cards(DayBefore)==CardsBefore,TEXT("new day's three legal cards preserve unfinished original source parameters")))return;
  if(!Check(FMath::IsNearlyEqual(W.VehicleFuel(),3.),TEXT("pump sequence starts from real default three-unit tank")))return;
  M->World->Enter(TEXT("road"));Go(10);return;
 }
 if(Stage==10)
 {
  if(M->World->bBusy||Age-At<.5)return;M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Go(11);return;
 }
 if(Stage==11)
 {
  if(M->World->bBusy||Age-At<.5)return;ParkAtPump();
  if(!Check(W.TripoPumpLoaded(),TEXT("actual private regular-pump model and trimmed station are mounted with their exact native asset paths; no fallback")))return;
  CashBefore=F.Budget();FuelBefore=W.VehicleFuel();AimAt(W.InteractionPoint(TEXT("ff:pump")));Go(111);return;
 }
 if(Stage==111)
 {
  if(Age-At<.3)return;AimAt(W.InteractionPoint(TEXT("ff:pump")));Go(12);return;
 }
 if(Stage==12)
 {
  if(Age-At<.9)return;FScreenshotRequest::RequestScreenshot(Root/TEXT("pump-before.png"),true,false);
  const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Picked==TEXT("ff:pump")||Picked==TEXT("town:pumps"),TEXT("native eye ray reaches actual regular pump meter: ")+Picked))return;
  M->Player->OnUse(Picked);
  if(!Check(W.PumpActive()&&F.Budget()==CashBefore&&W.VehicleFuel()==FuelBefore,TEXT("physical parked truck starts pump without up-front payment or fuel grant")))return;Go(13);return;
 }
 if(Stage==13)
 {
  if(W.PumpCharges()<1)return;
  if(!Check(W.PumpCharges()==1&&F.Budget()==CashBefore-1&&FMath::IsNearlyEqual(W.VehicleFuel(),FuelBefore+.25)&&FMath::IsNearlyEqual(W.PumpDelivered(),.25),TEXT("first completed metering interval charges once for actual delivered quarter-unit")))return;
  Go(14);return;
 }
 if(Stage==14)
 {
  if(Age-At<.6)return;FuelBefore=W.VehicleFuel();VolumeBefore=W.PumpDelivered();ProgressBefore=W.PumpProgress();ClockBefore=F.Clock();M->bPausedByMenu=true;Go(15);return;
 }
 if(Stage==15)
 {
  if(Age-At<.8)return;
  if(!Check(W.PumpActive()&&W.VehicleFuel()==FuelBefore&&W.PumpDelivered()==VolumeBefore&&FMath::IsNearlyEqual(W.PumpProgress(),ProgressBefore)&&F.Clock()==ClockBefore&&F.Budget()==CashBefore-1,TEXT("pause preserves incomplete pump interval without delivery, charge or clock advance")))return;
  M->bPausedByMenu=false;if(!Restore(TEXT("pump-mid-meter")))return;Go(16);return;
 }
 if(Stage==16)
 {
  // Validate immediately after normal Flow/startup; no fixture changes the meter.
  if(F.Director)F.Director->Stop();AimAt(W.InteractionPoint(TEXT("ff:pump")));
  if(!Check(W.PumpActive()&&W.VehicleFuel()==FuelBefore&&W.PumpDelivered()==VolumeBefore&&W.PumpCharges()==1&&F.Budget()==CashBefore-1,TEXT("Flow resumes paid receipt and tank without replaying a charge")))return;
  if(!Check(W.PumpProgress()+.001>=ProgressBefore&&W.PumpProgress()<.95,TEXT("Flow retains unfinished metering time rather than restarting a paid interval")))return;Go(17);return;
 }
 if(Stage==17)
 {
  if(W.PumpCharges()<3)return;W.StopPump();const double Fuel=W.VehicleFuel(),Volume=W.PumpDelivered();const int32 Cash=F.Budget(),Paid=W.PumpCharges();W.StopPump();W.StopPump();
  if(!Check(!W.PumpActive()&&Paid==3&&Cash==CashBefore-3&&FMath::IsNearlyEqual(Fuel,3.75)&&FMath::IsNearlyEqual(Volume,.75)&&W.VehicleFuel()==Fuel&&W.PumpDelivered()==Volume&&F.Budget()==Cash,TEXT("two further real delivery intervals and repeated stop retain exactly three charges and three quarters")))return;
  AimAt(W.InteractionPoint(TEXT("ff:pump")));Go(18);return;
 }
 if(Stage==18)
 {
  if(Age-At<.3)return;FScreenshotRequest::RequestScreenshot(Root/TEXT("pump-after.png"),true,false);Go(181);return;
 }
 if(Stage==181)
 {
  if(Age-At<.2)return;const FVector P=W.InteractionPoint(TEXT("ff:pump"));M->Player->Place(P.X+9.,P.Z,0);
  if(!Check(!W.StartPump()&&!W.PumpActive()&&F.Budget()==CashBefore-3,TEXT("player outside actual pump reach cannot start delivery or payment")))return;
  ParkAtPump();M->World->Roam(TEXT("diner"));Go(19);return;
 }
 if(Stage==19)
 {
  if(!M->World->bDriving){if(Age-At>10)Finish(false,TEXT("production vehicle boarding fixture did not enter driving"));return;}
  if(!Check(!W.StartPump()&&!W.PumpActive()&&F.Budget()==CashBefore-3,TEXT("production boarded vehicle rejects pump use without payment")))return;
  Go(191);return;
 }
 if(Stage==191)
 {
  if(Age-At<13.)return; // Let ordinary boarding/pump notices fade before the visual check.
  const auto* Truck=M->World->CabTruck();
  if(!Check(M->World->bDriving&&Truck&&Truck->bDriving&&Truck->TripoJeep()&&Truck->JeepCabReady(),TEXT("production driving fixture uses the real mounted Jeep interior and native driver seat without fallback")))return;
  const TCHAR* MeshNames[]={TEXT("SM_TripoJeep_InteriorBody"),TEXT("SM_TripoJeep_Interior"),TEXT("SM_TripoJeep_Steering"),TEXT("SM_TripoJeep_SpeedFace"),TEXT("SM_TripoJeep_FuelFace"),TEXT("SM_TripoJeep_TempFace"),TEXT("SM_TripoJeep_RadioFace")};
  for(const TCHAR* Name:MeshNames)
  {
   bool Found=false;
   Truck->Body.Traverse([&](FS47Obj O)
   {
    const auto* N=M->Web->Node(O.I);const auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
    const UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;
    // N1110: since the wheel split the body is SM_TripoJeep_WheelBody (Lived or TripoFleet), in place of the cab body (S47TripoFleet::InstallJeep)
    const bool bBodyName=FString(Name)==TEXT("SM_TripoJeep_InteriorBody")&&Mesh&&Mesh->GetName()==TEXT("SM_TripoJeep_WheelBody");
    if(Mesh&&(Mesh->GetName()==Name||bBodyName)&&(Mesh->GetPathName().StartsWith(TEXT("/Game/Licensed/TripoFleet/"))||Mesh->GetPathName().StartsWith(TEXT("/Game/Lived/")))&&Mesh->GetRenderData()&&Mesh->GetNumLODs()>0&&Mesh->GetNumTriangles(0)>0&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&O.Shown())Found=true;
   });
   if(!Check(Found,FString(TEXT("actual driving Jeep has a valid visible mounted native mesh: "))+Name))return;
  }
  FScreenshotRequest::RequestScreenshot(Root/TEXT("actual-jeep-driver.png"),true,false);Go(192);return;
 }
 if(Stage==192)
 {
  if(Age-At<.25)return;
  M->World->StopDriving();M->UI->Close(true);M->World->Enter(TEXT("saro"));Go(20);return;
 }
 if(Stage==20)
 {
  if(M->World->bBusy||Age-At<.5)return;
  if(!Check(F.AudioDesk&&F.AudioDesk->Ready(),TEXT("physical reel-deck adapter is ready in SARO")))return;
  int32 Pulsar=INDEX_NONE;for(int32 I=0;I<F.Jobs().Num();++I)if(F.Jobs()[I].Source==1&&F.Jobs()[I].Stage<=FS47FreeformSession::EJobStage::Pointed){Pulsar=I;break;}
  if(!Check(Pulsar!=INDEX_NONE&&F.SelectJob(Pulsar),TEXT("existing unfinished pulsar card selected without inserting completed work")))return;
  F.SetAnomaly(true);const auto* P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("selected production card supplies actual receiver profile")))return;
  auto& R=M->Story->Rx;R.Frequency=P->F;R.Gain=(P->Gain[0]+P->Gain[1])/2;R.Bandwidth=(P->Bw[0]+P->Bw[1])/2;R.Azimuth=(P->Az[0]+P->Az[1])/2;
  if(!Check(F.PointSelected(),TEXT("actual array receives the selected pulsar pointing command")))return;
  Go(21);return;
 }
 if(Stage==21)
 {
  if(!F.StartAcquisition())return;
  if(!Check(F.CurrentJob()->Stage==FS47FreeformSession::EJobStage::Integrating,TEXT("timed acquisition begins only after actual encoders settle")))return;
  Go(22);return;
 }
 if(Stage==22)
 {
  // GAMEPLAY-LOOP-01: no chart to fetch. RX-ANOMALY-FLOW-20261113: the take is retained at the receiver (RECORD tab) with its reference
  if(F.CurrentJob()->Stage!=FS47FreeformSession::EJobStage::Recorded)return;
  M->UI->Close(true);
  if(!Check(F.CurrentJob()->Elapsed==18&&FS47FreeformSession::RetainWaits(*F.CurrentJob())&&!F.CanRecordControl()&&F.PrinterJobId()!=F.CurrentJob()->Id,TEXT("real eighteen-second pulsar acquisition waits to be retained, with no chart at the printer")))return;
  if(!Check(F.RetainTake()&&F.CurrentJob()->bPrinted&&!F.CurrentJob()->OriginalPage.IsEmpty()&&F.CurrentJob()->bControl,TEXT("RETAIN TAKE keeps the original and starts its 8-second reference")))return;
  Go(23);return;
 }
 if(Stage==23)
 {
  const FVector A=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(A.X,A.Z-1.2,0);Go(24);return;
 }
 if(Stage==24)
 {
  if(Age-At<.35)return;
  if(!Check(F.AudioDesk->ListeningPosition(),TEXT("native equipment approach is inside powered physical listening range")))return;
  F.OpenAudio();if(!Check(!M->ModalOpen()&&!F.Held(),TEXT("audio arming opens no overlay and leaves the world active")))return;
  F.SeekAudio(9.1);F.AudioDesk->Use(TEXT("ff:audio:mark_start"));F.SeekAudio(13);F.AudioDesk->Use(TEXT("ff:audio:mark_end"));F.SeekAudio(18);F.AudioDesk->Use(TEXT("ff:audio:retain"));
  if(!Check(!F.CurrentJob()->bAudioReviewed&&F.CurrentJob()->OriginalCovered==0&&F.CurrentJob()->ControlCovered==0&&F.FeedbackText().Contains(TEXT("Not heard yet")),TEXT("instant seek and valid-looking marks cannot fabricate heard comparison; RETAIN says why")))return;
  F.AudioDesk->Use(TEXT("ff:audio:rewind"));if(!Check(F.AudioPosition()==0&&!F.AudioPlaying(),TEXT("physical rewind returns transport to zero without granting coverage")))return;
  AimAt(F.AudioDesk->InteractionPoint(TEXT("ff:audio:original")));Go(25);return;
 }
 if(Stage==25)
 {
  if(Age-At<.35)return;if(!UseAimedAudio(TEXT("ff:audio:original")))return;
  if(!Check(F.AudioSource()==0&&!M->ModalOpen(),TEXT("native ORIGINAL key selects production original without modal")))return;
  AimAt(F.AudioDesk->InteractionPoint(TEXT("ff:audio:play")));Go(26);return;
 }
 if(Stage==26)
 {
  if(Age-At<.35)return;if(!UseAimedAudio(TEXT("ff:audio:play")))return;
  if(!Check(F.AudioPlaying()&&!M->ModalOpen()&&F.AudioDesk->Label(TEXT("ff:audio:play")).Contains(TEXT("Stop")),TEXT("native PLAY key starts actual transport and updates its physical interaction status")))return;
  ClockBefore=F.Clock();Go(27);return;
 }
 if(Stage==27)
 {
  if(!bMarkedIn&&F.AudioPosition()>=9.1){F.AudioDesk->Use(TEXT("ff:audio:mark_start"));bMarkedIn=true;}
  if(!bMarkedOut&&F.AudioPosition()>=13){F.AudioDesk->Use(TEXT("ff:audio:mark_end"));bMarkedOut=true;}
  if(F.AudioPlaying())return;
  if(!Check(bMarkedIn&&bMarkedOut&&F.CurrentJob()->OriginalCovered==18&&F.CurrentJob()->MarkStart<=9.3&&F.CurrentJob()->MarkEnd>=12.6,TEXT("actual original playback covers eighteen seconds and physical keys retain measured marks")))return;
  if(!Check(F.Clock()>ClockBefore&&!M->ModalOpen(),TEXT("world time advances throughout physical listening")))return;
  // RX-ANOMALY-FLOW-20261113: the listen leaves RETAIN to the operator; the reference is on its own key
  if(!Check(!F.CurrentJob()->bAudioReviewed&&FS47FreeformSession::TapeHeard(*F.CurrentJob())&&F.CurrentJob()->ControlCovered==0,TEXT("one listen to the original leaves RETAIN to the operator")))return;
  F.AudioDesk->Use(TEXT("ff:audio:control"));
  if(!Check(F.AudioSource()==1&&!F.AudioPlaying(),TEXT("physical CONTROL key loads the take's 8-second reference")))return;
  F.AudioDesk->Use(TEXT("ff:audio:original"));
  F.AudioDesk->Use(TEXT("ff:audio:retain"));
  if(!Check(F.CurrentJob()->bAudioReviewed&&F.HasFinding(TEXT("pulse-review"))&&!M->ModalOpen()&&F.Budget()==CashBefore-3,TEXT("physical RETAIN keeps the heard take without modal or payment")))return;
  const FVector P=F.AudioDesk->InteractionPoint(TEXT("ff:audio:retain"));AimAt(P+FVector(0,.17,0));Go(29);return;
 }
 if(Stage==29)
 {
  if(Age-At<.4)return;FScreenshotRequest::RequestScreenshot(Root/TEXT("fysisk-audio.png"),true,false);Go(30);return;
 }
 if(Stage==30)
 {
  if(Age-At<.35)return;
  Finish(true);return;
 }
 if(Stage==38)
 {
  if(Age-At<2)return;
  if(M->Score&&M->Score->Playing.IsEmpty()&&Age-At<20)return;
  if(!Check(M->Score.IsValid()&&!M->Score->MusicHeld()&&M->Score->MusicWant()==TEXT("dawn"),TEXT("Freeform morning chooses its own music and is not held by Story state")))return;
  if(!Check(M->Score->Playing==TEXT("dawn")&&M->Score->ActiveDecks()>0,TEXT("ordinary Freeform update starts the imported music stream")))return;
  if(!Check(M->Audio&&M->Audio->SoundMixPlaying(TEXT("equipment:room_fluorescent")),TEXT("occupied Freeform room plays the staged fluorescent effect")))return;
  AimAt(F.AudioDesk->InteractionPoint(TEXT("ff:audio:ref_play")));
  FScreenshotRequest::RequestScreenshot(Root/TEXT("space-reference-desk.png"),true,false);
  Go(40);return;
 }
 if(Stage==40)
 {
  if(Age-At<.5)return;
  if(!Check(F.AudioDesk->ListeningPosition(),TEXT("powered reference desk reachable in actual SARO scene")))return;
  if(!UseAimedAudio(TEXT("ff:audio:ref_play")))return;
  if(!Check(F.AudioDesk->Label(TEXT("ff:audio:ref_play")).Contains(TEXT("Stop")),TEXT("native reference key starts staged recording")))return;
  if(!Check(M->Score&&M->Score->MusicHeld(),TEXT("Freeform reference listening holds the music")))return;
  Go(41);return;
 }
 if(Stage==41)
 {
  if(Age-At<.8)return;
  if(!Check(!F.AudioPlaying()&&F.CurrentJob()&&F.CurrentJob()->OriginalCovered==0&&F.CurrentJob()->ControlCovered==0&&!F.CurrentJob()->bAudioReviewed,TEXT("listening reference grants no acquisition or comparison coverage")))return;
  if(ScreenIndex==0)FScreenshotRequest::RequestScreenshot(Root/TEXT("space-reference-desk.png"),true,false);
  F.AudioDesk->Use(TEXT("ff:audio:ref_stop"));
  if(!Check(F.AudioDesk->Label(TEXT("ff:audio:ref_play")).Contains(TEXT("Play")),TEXT("reference stop returns physical transport to stopped state")))return;
  if(++ScreenIndex<6)
  {
   F.AudioDesk->Use(TEXT("ff:audio:ref_next"));AimAt(F.AudioDesk->InteractionPoint(TEXT("ff:audio:ref_play")));Go(40);return;
  }
  F.AudioDesk->Use(TEXT("ff:audio:ref_play"));
  const FVector P=F.AudioDesk->InteractionPoint(TEXT("ff:audio:ref_play"));M->Player->Place(P.X,P.Z-4,0);Go(42);return;
 }
 if(Stage==42)
 {
  if(Age-At<.5)return;
  if(!Check(!F.AudioDesk->ListeningPosition()&&F.AudioDesk->Label(TEXT("ff:audio:ref_play")).Contains(TEXT("Play")),TEXT("walking away stops reference transport")))return;
  if(!Check(M->Score&&!M->Score->MusicHeld(),TEXT("Freeform music is released after reference listening")))return;
  Finish(true,TEXT("six staged space references exercised through native SARO controls; no human listening acceptance"));return;
 }
 if(Stage==50)
 {
  if(Age-At<1)return;
  if(!Check(F.TerminalWindowRendered()&&M->Audio&&M->Audio->ReceiverMonitorOn(),TEXT("physical receiver starts continuous monitor through production Step")))return;
  if(!Check(M->Score&&!M->Score->MusicHeld()&&M->Score->MusicDuck()<.5,TEXT("search listening lowers the adaptive music without stopping it")))return;
  const auto* P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("receiver uses the current observing profile")))return;
  auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Gain=P->Gain[0];Rx.Bandwidth=P->Bw[0];Rx.Azimuth=(P->Az[0]+P->Az[1])*.5;
  Go(51);return;
 }
 if(Stage==51)
 {
  if(Age-At<.7)return;
  if(!Check(M->Story->Rx.Quality(*F.ConsoleProfile())>.99&&M->Audio->Carrier.Gain->Gain.Value()>.02,TEXT("meter lock produces strong carrier through live receiver update")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("receiver-search.png"),true,false);
  M->Story->Rx.Frequency=F.ConsoleProfile()->F+.4;Go(52);return;
 }
 if(Stage==52)
 {
  if(Age-At<.7)return;
  if(!Check(M->Audio->Carrier.Gain->Gain.Value()<.001&&M->Audio->Carrier.Stat->Gain.Value()>.005,TEXT("detuning replaces locked carrier with audible search static")))return;
  if(!OpenScreen(0))return;Go(53);return;
 }
 if(Stage==53)
 {
  if(Age-At<.7)return;
  if(!Check(!M->Audio->ReceiverMonitorOn()&&M->Audio->Carrier.Stat->Gain.Value()<.0001&&!M->Score->MusicHeld(),TEXT("switching workstations silences search and releases music")))return;
  if(!OpenScreen(3))return;Go(54);return;
 }
 if(Stage==54)
 {
  if(Age-At<.7)return;
  if(!Check(M->Audio->ReceiverMonitorOn(),TEXT("reopening receiver resumes continuous monitor")))return;
  F.SetPowerFault(true);Go(55);return;
 }
 if(Stage==55)
 {
  if(Age-At<.7)return;
  if(!Check(!M->Audio->ReceiverMonitorOn()&&M->Audio->Carrier.Stat->Gain.Value()<.0001,TEXT("feeder power loss silences receiver monitor")))return;
  F.SetPowerFault(false);M->UI->Close(true);
  M->Player->Place(60,60,0);M->Score->Held.FaceQuiet=[] { return 47.; };
  Go(56);return;
 }
 if(Stage==56)
 {
  if(Age-At<1)return;
  if(M->Score->Playing.IsEmpty()&&Age-At<20)return;
  if(!Check(M->Space==TEXT("yard")&&!M->Score->MusicHeld(),TEXT("outdoor Freeform music is released despite an inactive Story silence timer")))return;
  if(!Check(M->Score->Playing==M->Score->MusicWant()&&M->Score->ActiveDecks()>0,TEXT("ordinary outdoor update starts the imported score stream")))return;
  Finish(true,TEXT("receiver search and outdoor music stream passed; human listening remains unverified"));return;
 }

}
