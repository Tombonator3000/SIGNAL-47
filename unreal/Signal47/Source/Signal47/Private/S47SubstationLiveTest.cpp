#include "S47SubstationLiveTest.h"
#include "S47Substation.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Flow.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 using FSnapshot=FS47Substation::FFaultSnapshot;
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  bool Done=false,CameraInstalled=false,ServiceView=false,SideView=false,OriginalCalm=false,SaveQuarantined=false;
  FString Root,PendingImage;
  int32 Stage=0,AfterImage=0,InitialFuse=0,ShortBursts=0,OrdinaryContinues=0;
  FVector SavedFoot=FVector::ZeroVector;
  FString WrittenId;
  double Began=0,Since=0;
  FVector ServiceEye=FVector::ZeroVector;
  FSnapshot Normal,Fault,Repaired;
  TArray<TSharedPtr<FJsonValue>> Checks,Samples,FlowStates;
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> PreviousCamera;
  void Next(int32 S){Stage=S;Since=FPlatformTime::Seconds();UE_LOG(LogTemp,Display,TEXT("S47 substation live stage%d"),S);}
  void Finish(bool Good,const FString& Why)
  {
   if(Done)return;Done=true;auto* M=Main.Get();
   if(M&&M->Player&&CameraInstalled)M->Player->CameraOverride=MoveTemp(PreviousCamera);
   if(M&&M->Flow)M->Flow->bCalmFlash=OriginalCalm;
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("seconds"),Began>0?FPlatformTime::Seconds()-Began:0.);S47J::Set(O,TEXT("ordinaryContinues"),OrdinaryContinues);S47J::Set(O,TEXT("savedId"),WrittenId);O->SetArrayField(TEXT("checks"),Checks);O->SetArrayField(TEXT("samples"),Samples);O->SetArrayField(TEXT("flowStateSnapshots"),FlowStates);
   S47J::Set(O,TEXT("method"),TEXT("Isolated Freeform case0. Explicit diner on-foot area/initial existing service-apron placement, calibrated CameraOverride normal08 ramp/fault22 fence fixtures. Production TestAdvance selects22:00 only after the normal08 samples. Ordinary Main tick supplies every Substation update; this driver never calls Substation Step/StepVisuals. Actual live instance-matrix hashes/bounds, material/light/audio gain readback sampled at3 and8 seconds. SetPowerFault(true) is an explicit incident fixture. Actual native eye-ray Pick/Walker OnUse measures/isolates/replaces one stock fuse/starts the real three-second feeder test. Low and Calm visual states measured, SC02 burst count must remain unchanged. While isolated, an explicitly declared case1/save-store metadata fixture enables ordinary WriteSave, a fresh disk reader and title Continue; no feeder state is injected. Four native PNGs require root review. Normal08 ramp eye(7885.5,2.14,526) deliberately differs from the old BEFORE PropsAt7890;526 view to clear its sign; this is not an exact matched-pose image comparison."));
   S47J::Set(O,TEXT("limits"),TEXT("Automated camera/initial foot fixtures; no natural journey, human input/listening acceptance, full Story fault regression, pixel brightness guarantee or full-game matrix. No private feeder stage, completed finding, age or Substation state injected. Max90 seconds; expected about45 seconds plus one ordinary Continue."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 substation live %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
  }
  bool Check(bool Good,const FString& Text)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Text);Checks.Add(MakeShared<FJsonValueObject>(O));if(!Good)Finish(false,Text);return Good;
  }
  void StateSnapshot(const FString& Name)
  {
   auto* M=Main.Get();auto O=S47J::New();const FVector P=M->Player->Pos;
   S47J::Set(O,TEXT("phase"),Name);S47J::Set(O,TEXT("mode"),M->Mode==US47Main::EMode::Play?TEXT("play"):M->Mode==US47Main::EMode::Title?TEXT("title"):TEXT("other"));S47J::Set(O,TEXT("isFreeform"),M->IsFreeform());S47J::Set(O,TEXT("area"),M->World->Area);S47J::Set(O,TEXT("webArea"),M->Web->Area());S47J::Set(O,TEXT("caseId"),M->Flow->CaseId);S47J::Set(O,TEXT("devRun"),M->Flow->bDevRun);S47J::Set(O,TEXT("starting"),M->Flow->bStarting);S47J::Set(O,TEXT("restoring"),M->Flow->bRestoringNow);
   S47J::Set(O,TEXT("actualPos"),TArray<double>{P.X,P.Y,P.Z});S47J::Set(O,TEXT("savedPos"),TArray<double>{SavedFoot.X,SavedFoot.Y,SavedFoot.Z});S47J::Set(O,TEXT("actualPoseWalkable"),M->Player->Walkable(P.X,P.Z));S47J::Set(O,TEXT("savedPoseWalkable"),M->Player->Walkable(SavedFoot.X,SavedFoot.Z));S47J::Set(O,TEXT("poseDistance"),FVector::Dist(P,SavedFoot));
   S47J::Set(O,TEXT("powerFault"),M->Freeform->PowerFault());S47J::Set(O,TEXT("feederStage"),M->Freeform->Field->FeederStage());S47J::Set(O,TEXT("feederIsolated"),M->Freeform->Field->FeederIsolated());S47J::Set(O,TEXT("fuses"),M->Freeform->InventoryCount(TEXT("fuse")));S47J::Set(O,TEXT("initialFuses"),InitialFuse);
   FlowStates.Add(MakeShared<FJsonValueObject>(O));
   FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/(Name+TEXT(".json"))));
  }
  FSnapshot Sample(const FString& Name)
  {
   auto* M=Main.Get();const auto S=M->Substation->FaultSnapshot();auto O=S47J::New();
   S47J::Set(O,TEXT("name"),Name);S47J::Set(O,TEXT("age"),S.Age);S47J::Set(O,TEXT("actualFreeformClock"),M->Freeform->Clock());S47J::Set(O,TEXT("requested"),S.bRequested);S47J::Set(O,TEXT("ready"),S.bReady);S47J::Set(O,TEXT("active"),S.bActive);S47J::Set(O,TEXT("energized"),S.bEnergized);S47J::Set(O,TEXT("isolated"),S.bIsolated);S47J::Set(O,TEXT("fault"),S.bFault);S47J::Set(O,TEXT("held"),S.bHeld);S47J::Set(O,TEXT("arcVisible"),S.bArcVisible);S47J::Set(O,TEXT("lampsVisible"),S.bLampsVisible);S47J::Set(O,TEXT("materialAssigned"),S.bMaterialAssigned);S47J::Set(O,TEXT("safe"),S.bSafe);
   S47J::Set(O,TEXT("actualInstanceMatricesHash"),S.GeometryMotionHash);S47J::Set(O,TEXT("geometryBoundsValid"),bool(S.GeometryBounds.IsValid));
   const FVector Min=S.GeometryBounds.IsValid?S47Three::ToThree(S.GeometryBounds.Min):FVector::ZeroVector,Max=S.GeometryBounds.IsValid?S47Three::ToThree(S.GeometryBounds.Max):FVector::ZeroVector;
   S47J::Set(O,TEXT("geometryMinThree"),TArray<double>{Min.X,Min.Y,Min.Z});S47J::Set(O,TEXT("geometryMaxThree"),TArray<double>{Max.X,Max.Y,Max.Z});S47J::Set(O,TEXT("arcLumens"),S.ArcLumens);S47J::Set(O,TEXT("lampLumens"),S.LampLumens);S47J::Set(O,TEXT("arcIntensity"),S.ArcIntensity);S47J::Set(O,TEXT("lampIntensity"),S.LampIntensity);S47J::Set(O,TEXT("humGain"),S.HumGain);S47J::Set(O,TEXT("buzzGain"),S.BuzzGain);S47J::Set(O,TEXT("arcInstances"),S.ArcInstances);S47J::Set(O,TEXT("lampInstances"),S.LampInstances);
   const FVector Origin=S47Three::ToThree(S.Origin),Eye=S47Three::ToThree(M->Player->EyeUE()),Forward=S47Three::ToThree(M->Player->ForwardUE()*100.);
   S47J::Set(O,TEXT("originThree"),TArray<double>{Origin.X,Origin.Y,Origin.Z});S47J::Set(O,TEXT("actualEyeThree"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(O,TEXT("actualForwardThree"),TArray<double>{Forward.X,Forward.Y,Forward.Z});S47J::Set(O,TEXT("area"),M->World->Area);S47J::Set(O,TEXT("roadRootShown"),M->Web->Obj(TEXT("world.legs.road.area.group")).Shown());S47J::Set(O,TEXT("quality"),M->Flow->Quality);S47J::Set(O,TEXT("calm"),M->Flow->bCalmFlash);S47J::Set(O,TEXT("feederStage"),M->Freeform->Field->FeederStage());S47J::Set(O,TEXT("stockFuses"),M->Freeform->InventoryCount(TEXT("fuse")));S47J::Set(O,TEXT("SC02BurstCount"),M->Substation->ArcCount());
   TArray<UInstancedStaticMeshComponent*> Meshes;M->Web->GetComponents(Meshes);
   for(auto* C:Meshes)if(C&&C->GetName()==TEXT("SubstationFaultArc"))
   {
    S47J::Set(O,TEXT("actualArcComponent"),C->GetPathName());S47J::Set(O,TEXT("actualArcRegistered"),C->IsRegistered());
    if(auto* Mid=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))){const auto Color=Mid->K2_GetVectorParameterValue(TEXT("ArcColor"));S47J::Set(O,TEXT("actualMaterialColor"),TArray<double>{Color.R,Color.G,Color.B,Color.A});S47J::Set(O,TEXT("actualArcMaterial"),Mid->GetPathName());}
   }
   Samples.Add(MakeShared<FJsonValueObject>(O));return S;
  }
  bool Energized(const FSnapshot& S,bool Faulted)
  {
   auto* M=Main.Get();return Check(S.bReady&&S.bActive&&S.bEnergized&&!S.bIsolated&&S.bFault==Faulted&&!S.bHeld&&S.bArcVisible&&S.bMaterialAssigned&&S.bSafe&&S.ArcInstances>0&&S.ArcIntensity>0&&S.ArcLumens>0&&S.HumGain>0&&S.BuzzGain>0&&!S.GeometryMotionHash.IsEmpty()&&S.GeometryBounds.IsValid&&M->Web->Obj(TEXT("world.legs.road.area.group")).Shown(),Faulted?TEXT("actual energized fault state has visible live geometry/light/audio on the active road root"):TEXT("actual normal energized state has visible live geometry/light/audio on the active road root"));
  }
  bool Moving(const FSnapshot& Before,const FSnapshot& After,const FString& Name)
  {return Check(After.Age>Before.Age+2.5&&!After.GeometryMotionHash.IsEmpty()&&Before.GeometryMotionHash!=After.GeometryMotionHash,Name+TEXT(" actual ordinary-tick world instance matrices and age advance"));}
  void Shot(const FString& Name,int32 NextStage)
  {PendingImage=Root/(Name+TEXT(".png"));AfterImage=NextStage;FScreenshotRequest::RequestScreenshot(PendingImage,true,false);Next(90);}
  bool UseFeeder(int32 Expected)
  {
   auto* M=Main.Get();const FString Hit=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   if(!Check((Hit==TEXT("ff:feeder")||Hit==TEXT("substation:service")||Hit==TEXT("substation:lamp"))&&bool(M->Player->OnUse),TEXT("actual service eye ray hits feeder: ")+Hit))return false;
   M->Player->OnUse(Hit);return Check(M->Freeform->Field->FeederStage()==Expected,FString::Printf(TEXT("production feeder step advances to%d through native OnUse"),Expected));
  }
  void InstallCamera(US47Main* M,const TSharedPtr<FRun>& Self)
  {
    PreviousCamera=M->Player->CameraOverride;CameraInstalled=true;const TWeakPtr<FRun> Weak=Self;
    M->Player->CameraOverride=[Weak](double,const FVector2D&,const FVector2D&,FS47View& Out)
    {
     const auto S=Weak.Pin();auto* Owner=S?S->Main.Get():nullptr;if(!S||S->Done||!Owner||!Owner->Freeform||!Owner->Freeform->Field)return false;
     const FVector Target=S->ServiceView?Owner->Freeform->Field->InteractionPoint(TEXT("ff:feeder")):FVector(7890,6.6,539.45);
     Out.Eye=S->ServiceView?S->ServiceEye:S->SideView?FVector(7878,.52,535):FVector(7885.5,2.14,526);
     const FVector D=Target-Out.Eye;Out.Yaw=FMath::Atan2(-D.X,-D.Z);Out.Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
    };
  }
  void Update(AS47Web* W,const TSharedPtr<FRun>& Self)
  {
   if(Done)return;const double Now=FPlatformTime::Seconds();if(Began>0&&Now-Began>90){Finish(false,TEXT("bounded90-second native test timeout"));return;}
   auto* M=US47Main::Get(W);if(!M||!M->Flow||!M->World||!M->Player||!M->Freeform||!M->Substation||!M->Inter||!W->IsLoaded()||M->Flow->bStarting||S47Startup::Blocking())return;Main=M;
   if(Stage==0){Began=Now;OriginalCalm=M->Flow->bCalmFlash;US47Flow::FStart Start;Start.bDev=true;Start.bFreeform=true;M->Flow->StartGame(Start);Next(1);return;}
   if(Stage==91)
   {
    if(M->Mode!=US47Main::EMode::Title||Now-Since<.5)return;
    if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue action selects the saved isolated case")))return;
    Next(92);return;
   }
   if(!M->Freeform->Active()||!M->Freeform->Field||M->World->bBusy)return;
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Flow->bDevRun,TEXT("fresh isolated actual Freeform developer case0")))return;
    if(!Check(M->Flow->Quality==TEXT("low")&&M->Flow->Graphics.Effects==0,TEXT("required CLI -S47Quality=low selects actual Low effects profile")))return;
    M->UI->Close(true);M->World->Enter(TEXT("diner"));M->Flow->bCalmFlash=false;
    M->Player->Place(7885,525.8,0);M->Player->SetActive(false);M->Player->bHeadMotion=false;
    InitialFuse=M->Freeform->InventoryCount(TEXT("fuse"));
    StateSnapshot(TEXT("initial-existing-walkable-service-apron"));
    if(!Check(M->Player->Walkable(M->Player->Pos.X,M->Player->Pos.Z),TEXT("initial existing diner service-apron fixture is actually walkable")))return;
    InstallCamera(M,Self);
    ShortBursts=M->Substation->ArcCount();Next(2);return;
   }
   if(Stage==90)
   {
    TArray<uint8> PNG;if(FFileHelper::LoadFileToArray(PNG,*PendingImage)&&PNG.Num()>1024&&PNG[0]==137&&PNG[1]==80&&PNG[2]==78&&PNG[3]==71){Next(AfterImage);return;}
    if(Now-Since>6)Finish(false,TEXT("required native PNG unavailable: ")+PendingImage);return;
   }
   if(Stage==2)
   {
    if(Now-Since<3)return;if(!M->Substation->Ready())return;Normal=Sample(TEXT("normal-low-3s"));if(!Energized(Normal,false))return;Shot(TEXT("01-normal08-low-ramp-different-before-pose"),3);return;
   }
   if(Stage==3)
   {
    if(Now-Since<5)return;const auto S=Sample(TEXT("normal-low-8s"));if(!Energized(S,false)||!Moving(Normal,S,TEXT("Low normal")))return;
    Normal=S;const double ToNight=22*3600.-FMath::Fmod(M->Freeform->Clock(),86400.);
    if(!Check(ToNight>0&&M->Freeform->TestAdvance(ToNight),TEXT("production clock advance selects actual22:00 for fault/isolation/repair captures")))return;
    SideView=false;M->Freeform->SetPowerFault(true);Next(4);return;
   }
   if(Stage==4)
   {
    if(Now-Since<3)return;Fault=Sample(TEXT("fault-low-3s"));if(!Energized(Fault,true)||!Check(Fault.ArcIntensity>Normal.ArcIntensity&&Fault.BuzzGain>Normal.BuzzGain,TEXT("fault actually increases arc intensity and buzz over normal energized corona")))return;
    Shot(TEXT("02-fault22-low-ramp"),5);return;
   }
   if(Stage==5)
   {
    if(Now-Since<5)return;const auto S=Sample(TEXT("fault-low-8s"));if(!Moving(Fault,S,TEXT("Low fault")))return;
    ServiceView=true;ServiceEye=FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);Next(6);return;
   }
   if(Now-Since<.8)return;
   if(Stage==6){if(!UseFeeder(2))return;Next(7);return;}
   if(Stage==7){if(!UseFeeder(3))return;ServiceView=false;Next(8);return;}
   if(Stage==8)
   {
    if(Now-Since<1.5)return;const auto S=Sample(TEXT("isolated-low"));
    if(!Check(M->Freeform->PowerFault()&&M->Freeform->Field->FeederIsolated()&&S.bIsolated&&!S.bEnergized&&!S.bArcVisible&&FMath::IsNearlyZero(S.ArcLumens,.001f)&&FMath::IsNearlyZero(S.ArcIntensity,.001f)&&FMath::IsNearlyZero(S.HumGain,.001)&&FMath::IsNearlyZero(S.BuzzGain,.001),TEXT("actual enclosed isolation extinguishes geometry/light and both audio gains while fault is retained")))return;
    Shot(TEXT("03-isolated22-low-ramp"),9);return;
   }
   if(Stage==9)
   {
    if(!SaveQuarantined)
    {
     // Metadata-only fixture, following EvidenceTest: real current state is
     // written unmodified by production Flow into this isolated test profile.
     M->Flow->Saves=FS47SaveStore(Root/TEXT("SaveGames"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);SaveQuarantined=true;
    }
    if(!M->Flow->SaveBlock().IsEmpty()){if(Now-Since>8)Finish(false,TEXT("ordinary isolated save boundary refused: ")+M->Flow->SaveBlock());return;}
    SavedFoot=M->Player->Pos;StateSnapshot(TEXT("before-isolated-write-save"));
    if(!Check(M->Player->Walkable(SavedFoot.X,SavedFoot.Z),TEXT("saved isolated service pose passes the ordinary walkability rule")))return;
    FString Error;const auto Written=M->Flow->WriteSave(TEXT("manual"),0,Error);
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Choice=Reader.ContinueSave();const auto Disk=Choice?Reader.Load(Choice->Id):nullptr;
    if(!Check(Written&&Choice&&Disk&&Choice->Id==Written->Id&&S47J::Str(Disk,TEXT("sessionMode"))==TEXT("freeform"),TEXT("ordinary WriteSave and fresh reader select the same actual isolated Freeform case: ")+Error))return;
    const auto Free=S47J::Obj(Disk,TEXT("freeform")),Field=S47J::Obj(Free,TEXT("field"));
    if(!Check(S47J::Bool(Free,TEXT("powerFault"))&&S47J::Int(Field,TEXT("feeder"))==3&&S47J::Int(S47J::Obj(Field,TEXT("stock")),TEXT("fuse"))==InitialFuse,TEXT("fresh disk reader retains actual isolation and all unspent fuses")))return;
    WrittenId=Written->Id;FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("actual-isolated-continue-disk.json")));
    if(CameraInstalled){M->Player->CameraOverride=MoveTemp(PreviousCamera);CameraInstalled=false;}
    M->Flow->ToTitle();Next(91);return;
   }
   if(Stage==92)
   {
    if(Now-Since<1.5)return;
    StateSnapshot(TEXT("after-ordinary-continue-before-assertion"));
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->World->Area==TEXT("diner")&&M->Player->Walkable(M->Player->Pos.X,M->Player->Pos.Z)&&FVector::Dist(M->Player->Pos,SavedFoot)<.03&&M->Freeform->PowerFault()&&M->Freeform->Field->FeederStage()==3&&M->Freeform->Field->FeederIsolated()&&M->Freeform->InventoryCount(TEXT("fuse"))==InitialFuse,TEXT("ordinary title Continue restores actual isolated walkable diner-service pose and unspent inventory")))return;
    ++OrdinaryContinues;InstallCamera(M,Self);M->Player->SetActive(false);Next(93);return;
   }
   if(Stage==93)
   {
    if(Now-Since<1.5)return;const auto S=Sample(TEXT("isolated-after-ordinary-continue"));
    if(!Check(S.bIsolated&&!S.bEnergized&&!S.bArcVisible&&FMath::IsNearlyZero(S.ArcLumens,.001f)&&FMath::IsNearlyZero(S.ArcIntensity,.001f)&&FMath::IsNearlyZero(S.HumGain,.001)&&FMath::IsNearlyZero(S.BuzzGain,.001),TEXT("ordinary post-Continue Main tick keeps isolated arc/light/audio off")))return;
    ServiceView=true;Next(10);return;
   }
   if(Stage==10){if(!UseFeeder(4))return;if(!Check(M->Freeform->InventoryCount(TEXT("fuse"))==InitialFuse-1,TEXT("physical replacement consumes exactly one existing stock fuse")))return;Next(11);return;}
   if(Stage==11){if(!UseFeeder(4))return;Next(12);return;}
   if(Stage==12)
   {
    if(M->Freeform->PowerFault()){if(Now-Since>7)Finish(false,TEXT("real three-second feeder test did not complete"));return;}
    if(!Check(Now-Since>=2.8&&M->Freeform->Field->FeederStage()==5&&!M->Freeform->Field->FeederIsolated()&&M->Freeform->HasFinding(TEXT("feeder-repair")),TEXT("ordinary timed repair restores supply only after the real enclosed three-second test")))return;
    ServiceView=false;SideView=true;M->Flow->bCalmFlash=true;Next(13);return;
   }
   if(Stage==13)
   {
    if(Now-Since<3)return;Repaired=Sample(TEXT("repaired-low-calm-3s"));if(!Energized(Repaired,false))return;Next(14);return;
   }
   if(Stage==14)
   {
    if(Now-Since<5)return;const auto S=Sample(TEXT("repaired-low-calm-8s"));if(!Energized(S,false)||!Moving(Repaired,S,TEXT("Low Calm normal"))||!Check(M->Substation->ArcCount()==ShortBursts,TEXT("Freeform live visual driver never fires or replays the Story SC02 strike")))return;
    Shot(TEXT("04-repaired22-low-calm-fence"),15);return;
   }
   Finish(true,TEXT("ordinary-tick live normal/fault/isolated/repaired geometry and audio, native physical fuse repair after ordinary isolated disk/title Continue, Low/Calm motion and four PNGs passed; root image QA remains"));
  }
 };
}
void S47SubstationLiveTest::Update(AS47Web* Web)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47SubstationLiveTest")))return;
 static TSharedPtr<FRun> Run;
 if(!Run)
 {
  Run=MakeShared<FRun>();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/S47-substation-live-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
  Run->Root=FPaths::ProjectSavedDir()/TEXT("SubstationLiveTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 }
 Run->Update(Web,Run);
}
