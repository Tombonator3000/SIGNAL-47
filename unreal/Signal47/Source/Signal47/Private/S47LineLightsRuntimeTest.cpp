#include "S47SaroNight.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "S47Flow.h"
#include "S47LineLights.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "UnrealClient.h"

// Opt-in world/API contract. Truck movement and camera placement are explicit fixtures; visibility
// uses the actual camera and world traces, and Continue uses the real Writer/Reader/Flow path.
struct US47SaroNight::FLineContract
{
 bool bEnabled = false, bDone = false;
 int32 Stage = 0, Checks = 0, Loads = 0, Seen = 0, Notes = 0;
 double Elapsed = 0, Hold = 3;
 FString Root, Paused;
 TWeakObjectPtr<UStaticMeshComponent> Blocker;
};

void US47SaroNight::ControlLightsTest(double Dt)
{
 if (!LineContract)
 {
  LineContract = MakeShared<FLineContract>();
  LineContract->bEnabled = FParse::Param(FCommandLine::Get(),TEXT("S47LineContract"));
 }
 FLineContract& T = *LineContract;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [&T](const FString& Why)
 {
  T.bDone = true; if(T.Blocker.IsValid()) T.Blocker->DestroyComponent();
  UE_LOG(LogTemp,Error,TEXT("S47: line contract FAIL stage %d: %s"),T.Stage,*Why);
  FPlatformMisc::RequestExit(false);
 };
 if(!M || !M->Flow || !M->Story || !M->Player || !M->Web || !M->UI || !M->Night || !M->World)
 {Fail(TEXT("missing world dependency"));return;}
 if(!M->Flow->bDevRun || M->Flow->CaseId!=0)
 {Fail(TEXT("requires developer run without S47Case; never operates on a real case"));return;}
 T.Elapsed+=Dt;
 if(T.Elapsed>60){Fail(TEXT("60 second timeout"));return;}
 if(M->Flow->bStarting || M->World->bBusy) return;
 const auto Seen = [this]
 {
  int32 N=0;for(const auto& E:M->Night->SeenLog->Entries)if(E && E->Kind==TEXT("event") && E->What==TEXT("lights"))++N;
  return N;
 };
 const auto LineNotes = [this]
 {
  TArray<FString> Out;
  for(const FString& Note:M->Story->Notes())
   if(Note.Contains(TEXT("Three lights, low, west of the highway")) || Note.Contains(TEXT("Three low lights west of the highway"))) Out.Add(Note);
  return Out;
 };
 const auto Aim = [this](bool bTowards)
 {
  const FVector Target=LineLights->At().Get(FVector(-240,20.5,176));
  const FVector Dir=(Target-Camera().Pos).GetSafeNormal()*(bTowards?1:-1);
  M->Player->Yaw=FMath::Atan2(-Dir.X,-Dir.Z);M->Player->Pitch=FMath::Asin(Dir.Y);
 };
 if(T.Stage>=2 && T.Stage<=7) Aim(true);
 if(T.Hold>0){T.Hold-=Dt;return;}
 const auto Check=[&T,&Fail](bool Good,const FString& What)
 {
  ++T.Checks;if(!Good){Fail(What);return false;}
  UE_LOG(LogTemp,Display,TEXT("S47: line contract PASS: %s"),*What);return true;
 };
 const auto Stage=[&T](int32 Next,double Hold=.2){T.Stage=Next;T.Hold=Hold;};
 const auto Disk=[this,&T,&Fail,&Check](const FString& Name,const S47J::FObj& State)
 {
  FS47SaveStore Writer(T.Root/Name);Writer.Init();FS47SaveCard Card;
  Card.Chapter=TEXT("Line lights world contract");Card.Place=Area();Card.Clock=S47Text::ClockText(Clock(),true);
  FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,State,Card,Error);
  if(!Meta){Fail(TEXT("isolated Writer failed: ")+Error);return false;}
  FS47SaveStore Reader(T.Root/Name);Reader.Init();const auto Loaded=Reader.Load(Meta->Id);
  if(!Loaded){Fail(TEXT("fresh Reader failed: ")+Name);return false;}
  FFileHelper::SaveStringToFile(S47J::Write(Loaded,true),*(T.Root/(Name+TEXT(".loaded.json"))));
  auto ExpectedNotes=S47J::Strs(S47J::Obj(Loaded,TEXT("case")),TEXT("notes"));
  if(S47J::Int(Loaded,TEXT("timelineVersion"),1)!=2)
   ExpectedNotes.AddUnique(TEXT("Legacy case timeline retained: original 02:13 references and photographs are unchanged. Any unstored printout acquisition time is marked unavailable."));
  US47Flow::FStart Start;Start.CaseId=0;Start.bDev=true;Start.State=Loaded;Start.Playtime=M->Playtime;
  M->Flow->StartGame(Start);
  if(!Check(M->Story->Notes()==ExpectedNotes,TEXT("full Flow preserves saved notes plus only the explicit legacy timeline migration note: ")+Name))return false;
  ++T.Loads;return true;
 };
 if(T.Stage==0)
 {
  T.Root=FPaths::ProjectSavedDir()/TEXT("Screenshots")/(TEXT("line-contract-")+FGuid::NewGuid().ToString(EGuidFormats::Digits));
  FParse::Value(FCommandLine::Get(),TEXT("S47Evidence="),T.Root);
  IFileManager::Get().MakeDirectory(*T.Root,true);
  if(!Check(Phase()==TEXT("ch1") && Area()==TEXT("saro") && !Driving() && !M->TownSeen.Contains(TEXT("ev:lights")),TEXT("isolated chapter-one night starts without a consumed line event")))return;
  LineLights->Reset(false);M->Player->Place(-24,130,-UE_DOUBLE_PI/2);M->Player->Pitch=0;
  TestTruck=FVector(-24,0,130);FS47LineLightsIn S;S.bOk=true;S.Truck=TestTruck;S.Ground=[](double,double){return 0.0;};
  LineLights->Now(S);T.Seen=Seen();T.Notes=LineNotes().Num();Aim(false);Stage(1,1.2);return;
 }
 if(T.Stage==1)
 {
  if(!Check(LineLights->bStarted && !LineLights->bObserved && !LineLights->bDone && Seen()==T.Seen && LineNotes().Num()==T.Notes,TEXT("offscreen world occurrence produces no line journal, seen credit or consumed flag")))return;
  UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  if(!Cube){Fail(TEXT("opaque sight fixture mesh unavailable"));return;}
  UStaticMeshComponent* Block=NewObject<UStaticMeshComponent>(M->Web);Block->SetStaticMesh(Cube);
  Block->SetupAttachment(M->Web->GetRootComponent());Block->SetWorldLocation(S47Three::ToUE((Camera().Pos+LineLights->At().GetValue())*.5));
  Block->SetWorldScale3D(FVector(80));Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Block->SetCollisionResponseToAllChannels(ECR_Ignore);Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Block->RegisterComponent();
  T.Blocker=Block;Aim(true);Stage(2,1.2);return;
 }
 if(T.Stage==2)
 {
  if(!Check(!LineLights->bObserved && Seen()==T.Seen && !M->TownSeen.Contains(TEXT("ev:lights")),TEXT("an actual opaque world mesh blocks journal/seen observation while the camera faces the lights")))return;
  T.Blocker->DestroyComponent();T.Blocker.Reset();Stage(3,1.2);return;
 }
 if(T.Stage==3)
 {
  if(!Check(LineLights->bObserved && !LineLights->bDone && Seen()==T.Seen+1 && LineNotes().Num()==T.Notes,TEXT("clear actual camera sight creates exactly one observed entry without a fictional ending")))return;
  FScreenshotRequest::RequestScreenshot(T.Root/TEXT("observed-world.png"),false,false);
  if(!Disk(TEXT("observed-active"),M->Flow->GameState()))return;
  TestTruck=FVector(-24,0,S47J::Num(LineLights->Save(),TEXT("lastZ")));Stage(4,.4);return;
 }
 if(T.Stage==4)
 {
  UE_LOG(LogTemp,Display,TEXT("S47: line contract restored state=%s observed=%d done=%d seen=%d expected=%d lineNotes=%d expected=%d totalNotes=%d"),*LineLights->State,LineLights->bObserved,LineLights->bDone,Seen(),T.Seen+1,LineNotes().Num(),T.Notes,M->Story->Notes().Num());
  if(!Check(LineLights->bObserved && !LineLights->bDone && LineLights->State==TEXT("pace") && Seen()==T.Seen+1 && LineNotes().Num()==T.Notes,TEXT("fresh Writer/Reader and full Flow Continue preserve real observation and its active phase")))return;
  FS47Doc Doc;Doc.Id=TEXT("LINE_CONTRACT_PAUSE");Doc.Kind=TEXT("typed");Doc.Title=TEXT("Field note");Doc.Page=TEXT("The night waits while this document is open.");
  M->UI->Document(Doc);T.Paused=S47J::Write(LineLights->Save());Stage(5,.6);return;
 }
 if(T.Stage==5)
 {
  if(!Check(M->UI->Modal() && S47J::Write(LineLights->Save())==T.Paused && Seen()==T.Seen+1 && LineNotes().Num()==T.Notes,TEXT("a real modal document holds phase, world positions and line journal state")))return;
  M->UI->Close();Stage(6,.15);return;
 }
 if(T.Stage==6){TestTruck.Reset();Stage(7,.2);return;}
 if(T.Stage==7)
 {
  const auto Notes=LineNotes();
  if(!Check(LineLights->bDone && LineLights->bObserved && !LineLights->bCompleted && LineLights->State==TEXT("none") && M->TownSeen.Contains(TEXT("ev:lights")) && Seen()==T.Seen+1 && Notes.Num()==T.Notes+1 && Notes.Last().Contains(TEXT("lost sight")) && !Notes.Last().Contains(TEXT("Then they turned off")),TEXT("truck loss after real sight writes one honest partial note and consumes only this legitimate sighting")))return;
  if(!Disk(TEXT("observed-aborted"),M->Flow->GameState()))return;Stage(8,.3);return;
 }
 if(T.Stage==8)
 {
  if(!Check(LineLights->bDone && LineLights->bObserved && Seen()==T.Seen+1 && LineNotes().Num()==T.Notes+1,TEXT("Continue after abort preserves the real note without duplicate callback or replay")))return;
  const auto Legacy=M->Flow->GameState();Legacy->RemoveField(TEXT("lineLights"));
  if(!Disk(TEXT("legacy-consumed"),Legacy))return;Stage(9,.3);return;
 }
 if(T.Stage==9)
 {
  if(!Check(LineLights->bDone && !LineLights->bObserved && LineLights->State==TEXT("none") && Seen()==T.Seen+1 && LineNotes().Num()==T.Notes+1,TEXT("legacy consumed saves stay consumed while new observed credit remains unknown")))return;
  auto Report=S47J::New();S47J::Set(Report,TEXT("pass"),true);S47J::Set(Report,TEXT("checks"),T.Checks);S47J::Set(Report,TEXT("diskRestores"),T.Loads);S47J::Set(Report,TEXT("seenEntries"),Seen()-T.Seen);
  FFileHelper::SaveStringToFile(S47J::Write(Report,true),*(T.Root/TEXT("result.json")));
  T.bDone=true;UE_LOG(LogTemp,Display,TEXT("S47: line contract PASS complete: %d checks, %d fresh disk/full-Flow restores, actual world occlusion and modal/abort/legacy; evidence %s"),T.Checks,T.Loads,*T.Root);
  FPlatformMisc::RequestExit(false);
 }
}
