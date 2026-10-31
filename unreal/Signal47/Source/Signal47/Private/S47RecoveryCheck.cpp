#include "S47RecoveryCheck.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47Spine.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47Evening.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

namespace S47RecoveryCheck
{
bool Run(US47Main* M)
{
 if (!FParse::Param(FCommandLine::Get(),TEXT("S47RecoveryCheck"))) return true;
 bool Ok=true;int32 Count=0;
 const auto Check=[&](bool Good,const TCHAR* What)
 {
  ++Count;Ok&=Good;
  UE_LOG(LogTemp,Display,TEXT("S47: recovery %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),What);
  return Good;
 };
 FString UserDir,Open;
 const bool Explicit=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
 FParse::Value(FCommandLine::Get(),TEXT("S47Open="),Open);
 UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 if (!Check(Explicit && (UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))
  && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
  && FParse::Param(FCommandLine::Get(),TEXT("S47Title")) && FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest")) && Open==TEXT("continue"),
  TEXT("explicit isolated temporary UserDir, Title, ordinary Continue and SmokeTest are required"))) return false;
 if (!Check(M && M->Flow && M->Story && M->Horror && M->Horror->SC03 && M->Spine && M->World && M->Inter,
  TEXT("the ordinary loaded game owns all recovery systems"))) return false;
 const auto Original=M->Flow->Carry;
 const auto OriginalSC03=S47J::Obj(S47J::Obj(Original,TEXT("horror")),TEXT("sc03"));
 FS47HorrorSC03::EPhase Phase;FS47HorrorSC03::FFacts Facts;
 if (!Check(Original && !Original->HasField(TEXT("spine")) && FS47HorrorSC03::Read(OriginalSC03,Phase,Facts)
  && Phase==FS47HorrorSC03::EPhase::Complete && Facts.bObserved && Facts.bMissing && Facts.bLocalOrder
  && !Facts.MissingStamp.IsEmpty() && !Facts.OrderStamp.IsEmpty(),TEXT("source copy contains the actual completed deputy boundary and no Spine field"))) return false;
 Check(M->Mode==US47Main::EMode::Play && M->Flow->CaseId>0 && !M->Flow->bStarting && !M->Flow->bRestoringNow,
  TEXT("Continue has finished loading a real case without developer jump"));
 Check(M->Horror->SC03Complete() && S47J::Write(M->Horror->SC03->Save())==S47J::Write(FS47HorrorSC03::Write(Phase,Facts)),
  TEXT("restored SC03 facts and all report stamps are unchanged"));
 Check(M->Spine->Enabled() && M->Spine->Gate(FS47Spine::SC04)==1 && M->Spine->Pending()==FS47Spine::SC04
  && M->Story->ChapterKey==TEXT("prologue") && M->Story->Phase()==ES47Phase::Residual,
  TEXT("the missing route resumes exactly at active SC04 in the residual prologue"));
 Check(M->World->Area==TEXT("diner") && M->World->TruckAt==TEXT("diner") && !M->World->bDriving && !M->World->bBusy,
  TEXT("the original Caliche parked vehicle and on-foot scene are retained"));
 const auto Choice=M->World->TruckChoice?M->World->TruckChoice(TEXT("diner")):US47World::FChoice();
 Check(Choice.Label==TEXT("Return to SARO") && bool(Choice.Use),TEXT("the actual parked-vehicle choice offers Return to SARO"));
 Check(M->Spine->Objective().Contains(TEXT("SARO")),TEXT("the next authored objective directs the player back to SARO"));
 Check(M->World->PersonalUnlocked() && !M->World->PersonalTransport() && M->World->PersonalPlace()==TEXT("saro")
  && M->World->ServicePlace()==TEXT("diner"),TEXT("both vehicles exist independently: own car at SARO, service truck at Caliche"));
 const auto Pose=S47J::Obj(Original,TEXT("truckPose"));
 const FVector OldPos(S47J::Num(Pose,TEXT("x")),S47J::Num(Pose,TEXT("y")),S47J::Num(Pose,TEXT("z")));
 Check(Pose && M->World->ServicePosition().Equals(OldPos,.01),TEXT("the service truck has not teleported from its copied saved pose"));
 const auto Transport=M->World->TransportSave();
 Check(Transport && S47J::Obj(Transport,TEXT("personal")) && S47J::Obj(Transport,TEXT("service"))
  && S47J::Str(S47J::Obj(Transport,TEXT("personal")),TEXT("at"))==TEXT("saro")
  && S47J::Str(S47J::Obj(Transport,TEXT("service")),TEXT("at"))==TEXT("diner"),
  TEXT("the composed two-car save keeps both independent parking records"));
 const auto* Personal=M->Inter->Get(TEXT("personalVehicle"));
 const auto* Service=M->Inter->Get(TEXT("dinerTruck"));
 Check(Personal && Personal->Object,TEXT("the personal car owns a registered native use target"));
 Check(Service && Service->Object,TEXT("the parked service truck owns its registered production use target"));
 Check(M->Inter->LabelOf(TEXT("dinerTruck"))==TEXT("Return to SARO"),TEXT("the native service-truck target retains the recovered Return to SARO label"));
 UE_LOG(LogTemp,Display,TEXT("S47: recovery %s, %d checks; choice '%s', objective '%s', personal %s service %s"),
  Ok?TEXT("PASS"):TEXT("FAIL"),Count,*Choice.Label,*M->Spine->Objective(),*M->World->PersonalPosition().ToString(),*M->World->ServicePosition().ToString());
 return Ok;
}
}
