#include "S47FreeformInWorldTest.h"
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
 if(Good)for(const TCHAR* Name:{TEXT("operations.png"),TEXT("status.png"),TEXT("tracking.png"),TEXT("receiver.png"),TEXT("pump-before.png"),TEXT("pump-after.png")})
 {
  TArray<uint8> Bytes;const bool PNG=FFileHelper::LoadFileToArray(Bytes,*(Root/Name))&&Bytes.Num()>1000&&Bytes[0]==137&&Bytes[1]==80&&Bytes[2]==78&&Bytes[3]==71;
  if(!Check(PNG,FString(TEXT("native screenshot exists with PNG signature: "))+Name)){Good=false;}
 }
 if(!Good&&Why.IsEmpty())Why=TEXT("required native screenshot was not captured");
 if(M){M->bPausedByMenu=false;if(M->UI)M->UI->Close(true);}
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("flowRestores"),Restores);S47J::Set(O,TEXT("elapsedActiveDriverSeconds"),Age);O->SetArrayField(TEXT("evidence"),Evidence);
 S47J::Set(O,TEXT("method"),TEXT("Production OpenOperations/Status/Tracking/Receiver callbacks, actual native CRT geometry/camera transition and rendered window, actual active clock, paid pump metering, pause and full disk/Flow restore. Source parameter cards retained before/after save and explicit one-day advancement."));
 S47J::Set(O,TEXT("limits"),TEXT("Isolated /tmp developer case 0 only; explicit camera/player/vehicle approach fixtures replace natural navigation and long-distance driving. No claim of actual mouse navigation or human readability acceptance. No full-night test or whole-game regression. Bounded below 180 seconds."));
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
 Age+=FMath::Clamp(Dt,0.,.2);if(Age>175){Finish(false,TEXT("bounded in-world test timeout"));return;}
 if(!M||!M->Flow||!M->Freeform||!M->Freeform->Active()||!M->Freeform->Field||!M->Web||!M->Web->IsLoaded()||!M->World||!M->Player||M->Flow->bStarting||S47Startup::Blocking())return;
 auto& F=*M->Freeform;auto& W=*F.Field;
 if(Stage==0)
 {
  if(Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->IsFreeform(),TEXT("temporary developer Freeform case 0 required")))return;
  if(F.Director)F.Director->Stop();DayBefore=F.Day();CardsBefore=Cards(DayBefore);CaptureCards(TEXT("before-save"));
  if(!Check(F.Jobs().Num()>=3&&!CardsBefore.IsEmpty(),TEXT("daily source parameter cards exist in production session")))return;
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
  if(M->World->bBusy||Age-At<.5)return;ParkAtPump();CashBefore=F.Budget();FuelBefore=W.VehicleFuel();AimAt(W.InteractionPoint(TEXT("ff:pump")));Go(12);return;
 }
 if(Stage==12)
 {
  if(Age-At<.9)return;FScreenshotRequest::RequestScreenshot(Root/TEXT("pump-before.png"),true,false);
  if(!Check(W.StartPump()&&W.PumpActive()&&F.Budget()==CashBefore&&W.VehicleFuel()==FuelBefore,TEXT("physical parked truck starts pump without up-front payment or fuel grant")))return;Go(13);return;
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
  if(F.Director)F.Director->Stop();
  if(!Check(W.PumpActive()&&W.VehicleFuel()==FuelBefore&&W.PumpDelivered()==VolumeBefore&&W.PumpCharges()==1&&F.Budget()==CashBefore-1,TEXT("Flow resumes paid receipt and tank without replaying a charge")))return;
  if(!Check(W.PumpProgress()+.001>=ProgressBefore&&W.PumpProgress()<.95,TEXT("Flow retains unfinished metering time rather than restarting a paid interval")))return;Go(17);return;
 }
 if(Stage==17)
 {
  if(W.PumpCharges()<3)return;W.StopPump();const double Fuel=W.VehicleFuel(),Volume=W.PumpDelivered();const int32 Cash=F.Budget(),Paid=W.PumpCharges();W.StopPump();W.StopPump();
  if(!Check(!W.PumpActive()&&Paid==3&&Cash==CashBefore-3&&FMath::IsNearlyEqual(Fuel,3.75)&&FMath::IsNearlyEqual(Volume,.75)&&W.VehicleFuel()==Fuel&&W.PumpDelivered()==Volume&&F.Budget()==Cash,TEXT("two further real delivery intervals and repeated stop retain exactly three charges and three quarters")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("pump-after.png"),true,false);Go(18);return;
 }
 if(Stage==18)
 {
  if(Age-At<.3)return;const FVector P=W.InteractionPoint(TEXT("ff:pump"));M->Player->Place(P.X+9.,P.Z,0);
  if(!Check(!W.StartPump()&&!W.PumpActive()&&F.Budget()==CashBefore-3,TEXT("player outside actual pump reach cannot start delivery or payment")))return;
  ParkAtPump();M->World->Roam(TEXT("diner"));Go(19);return;
 }
 if(Stage==19)
 {
  if(!M->World->bDriving){if(Age-At>10)Finish(false,TEXT("production vehicle boarding fixture did not enter driving"));return;}
  if(!Check(!W.StartPump()&&!W.PumpActive()&&F.Budget()==CashBefore-3,TEXT("production boarded vehicle rejects pump use without payment")))return;
  M->World->StopDriving();Finish(true);return;
 }
}
