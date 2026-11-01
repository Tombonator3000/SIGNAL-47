#include "S47SaroWallTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Graphics.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47UI.h"
#include "Containers/Ticker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
 const TCHAR* Base=TEXT("/home/tom/scratch-bm/Signal47-saro-wall-investigation-20261101/");
 constexpr double Evening=23*3600+49*60;
 S47J::FObj Vec(const FVector& V)
 {auto O=S47J::New();S47J::Set(O,TEXT("x"),V.X);S47J::Set(O,TEXT("y"),V.Y);S47J::Set(O,TEXT("z"),V.Z);return O;}
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  FString Fixture,Output;
  int32 Stage=0;
  double Age=0,At=0;
  double X=-.9,Z=5.15,Yaw=-2.75,Pitch=-.04;
  TArray<TSharedPtr<FJsonValue>> Frames;
  bool bRestAccepted=false;
  bool Finish(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("diagnosticComplete"),Good);S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("fixture"),Fixture);S47J::Set(O,TEXT("restAccepted"),bRestAccepted);
   S47J::Set(O,TEXT("scope"),TEXT("Nearby saved state, explicitly staged fixed corridor pose and Low graphics. Clock-only evening/morning and real Rest API; no natural navigation or exact user-error frame claimed. Screenshots and node records require comparison; completion is not a wall-fix PASS."));
   O->SetArrayField(TEXT("frames"),Frames);
   if(!Output.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Output/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 wall diagnostic %s: %s"),Good?TEXT("COMPLETE"):TEXT("FAILED"),*Why);
   FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Capture(const FString& Name)
  {
   auto* M=Main.Get();auto* W=M->Web.Get();auto O=S47J::New();
   S47J::Set(O,TEXT("phase"),Name);S47J::Set(O,TEXT("storyClock"),M->Story->Clock());S47J::Set(O,TEXT("freeformClock"),M->Freeform->Clock());
   S47J::Set(O,TEXT("player"),Vec(M->Player->Pos));S47J::Set(O,TEXT("eye"),Vec(S47Three::ToThree(M->Player->EyeUE())));
   S47J::Set(O,TEXT("yaw"),M->Player->Yaw);S47J::Set(O,TEXT("pitch"),M->Player->Pitch);
   S47J::Set(O,TEXT("annexInteriorShown"),W->Obj(TEXT("annex.interior")).Shown());
   S47J::Set(O,TEXT("graphics"),W->GetGraphicsSettings().PresetId());
   const FBox Region(FVector(-7,-.5,4.4),FVector(3,3.6,11));
   TArray<TSharedPtr<FJsonValue>> Nodes;
   for(int32 I=0;I<W->PartCount();++I)
   {
    const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
    UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;if(!Mesh)continue;
    const FBox UB=Mesh->GetBoundingBox().TransformBy(C->GetComponentTransform());
    const FVector A=S47Three::ToThree(UB.Min),B=S47Three::ToThree(UB.Max);
    const FBox Bounds(A.ComponentMin(B),A.ComponentMax(B));if(!Bounds.Intersect(Region))continue;
    auto R=S47J::New();S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("parent"),N->Parent);S47J::Set(R,TEXT("name"),N->Name);
    S47J::Set(R,TEXT("component"),C->GetName());S47J::Set(R,TEXT("mesh"),Mesh->GetPathName());
    S47J::Set(R,TEXT("localVisible"),N->bVis);S47J::Set(R,TEXT("shown"),W->ObjAt(I).Shown());
    S47J::Set(R,TEXT("componentVisible"),C->IsVisible());S47J::Set(R,TEXT("hiddenInGame"),C->bHiddenInGame);
    S47J::Set(R,TEXT("min"),Vec(Bounds.Min));S47J::Set(R,TEXT("max"),Vec(Bounds.Max));
    S47J::Set(R,TEXT("lods"),Mesh->GetNumLODs());S47J::Set(R,TEXT("lod0Triangles"),Mesh->GetNumTriangles(0));
    TArray<TSharedPtr<FJsonValue>> Mats;
    for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* Mat=C->GetMaterial(Slot))
    {
     auto T=S47J::New();S47J::Set(T,TEXT("slot"),Slot);S47J::Set(T,TEXT("path"),Mat->GetPathName());
     S47J::Set(T,TEXT("blendMode"),int32(Mat->GetBlendMode()));S47J::Set(T,TEXT("twoSided"),Mat->IsTwoSided());
     for(const TCHAR* Key:{TEXT("Opacity"),TEXT("NativeLighting"),TEXT("Cutout"),TEXT("OpacityMask")})
     {float V=0;if(Mat->GetScalarParameterValue(FMaterialParameterInfo(Key),V))S47J::Set(T,Key,double(V));}
     Mats.Add(MakeShared<FJsonValueObject>(T));
    }
    R->SetArrayField(TEXT("materials"),Mats);Nodes.Add(MakeShared<FJsonValueObject>(R));
   }
   O->SetArrayField(TEXT("nearbyMeshes"),Nodes);Frames.Add(MakeShared<FJsonValueObject>(O));
   const bool Written=FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Output/(Name+TEXT(".json"))));
   FScreenshotRequest::RequestScreenshot(Output/(Name+TEXT(".png")),true,false);
   UE_LOG(LogTemp,Display,TEXT("S47 wall diagnostic %s: %d mesh records, annex shown=%d, StoryClock=%.1f"),*Name,Nodes.Num(),W->Obj(TEXT("annex.interior")).Shown(),M->Story->Clock());
   return Written;
  }
  void Next(int32 S){Stage=S;At=Age;}
  bool Step(float Dt)
  {
   Age+=Dt;if(Age>90)return Finish(false,TEXT("timeout"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Story||!M->Freeform||S47Startup::Blocking()||M->Flow->bStarting)return true;
   if(Stage==0)
   {
    FString Text;if(!FFileHelper::LoadFileToString(Text,*Fixture))return Finish(false,TEXT("fixture unavailable"));
    const auto Envelope=S47J::Read(Text);auto State=S47J::Obj(Envelope,TEXT("state"));
    if(!State||S47J::Str(State,TEXT("sessionMode"))!=TEXT("freeform")||S47J::Str(State,TEXT("area"))!=TEXT("saro"))return Finish(false,TEXT("requires nearby SARO Freeform fixture"));
    auto Free=S47J::Obj(State,TEXT("freeform"));if(!Free)return Finish(false,TEXT("fixture missing Freeform state"));
    S47J::Set(Free,TEXT("clock"),Evening);
    M->Flow->Graphics=FS47GraphicsSettings::ForPreset(TEXT("low"));
    M->Web->ApplyGraphicsSettings(M->Flow->Graphics);
    US47Flow::FStart Start;Start.bDev=true;Start.State=State;M->Flow->StartGame(Start);Next(1);return true;
   }
   if(!M->IsFreeform()||!M->Freeform->Active()||M->Flow->CaseId!=0)return Finish(false,TEXT("isolated developer Freeform case0 required"));
   if(Stage==1)
   {
    if(Age-At<2)return true;M->UI->Close(true);M->bPausedByMenu=true;
    M->Player->Place(X,Z,Yaw);M->Player->Pitch=Pitch;M->Player->bHeadMotion=false;M->Player->SetActive(false);
    M->Story->SetClock(Evening);Next(2);return true;
   }
   // Keep only the simulation clock held. WorldLife and visibility still tick normally.
   M->bPausedByMenu=true;M->Player->SetActive(false);
   if(Age-At<3)return true;
   if(Stage==2){if(!Capture(TEXT("01-evening")))return Finish(false,TEXT("snapshot write failed"));Next(3);return true;}
   if(Stage==3){M->Story->SetClock(8*3600);Next(4);return true;}
   if(Stage==4){if(!Capture(TEXT("02-clock-only-morning")))return Finish(false,TEXT("snapshot write failed"));Next(5);return true;}
   if(Stage==5){M->Story->SetClock(Evening);Next(6);return true;}
   if(Stage==6){if(!Capture(TEXT("03-clock-only-evening")))return Finish(false,TEXT("snapshot write failed"));Next(7);return true;}
   if(Stage==7)
   {
    // The production Rest button runs with a modal open. Use that same precondition
    // without moving the player to OPS, so this comparison isolates the wall state.
    M->UI->Open(SNew(SBorder));bRestAccepted=M->Freeform->Rest();
    if(!bRestAccepted){M->UI->Close(true);return Finish(false,TEXT("production Rest refused fixture (active work/event); do not falsify it"));}
    Next(8);return true;
   }
   if(Stage==8){if(!Capture(TEXT("04-production-rest")))return Finish(false,TEXT("snapshot write failed"));Next(9);return true;}
   for(const TCHAR* Name:{TEXT("01-evening"),TEXT("02-clock-only-morning"),TEXT("03-clock-only-evening"),TEXT("04-production-rest")})
    if(!IFileManager::Get().FileExists(*(Output/(FString(Name)+TEXT(".png")))))return Finish(false,TEXT("screenshot missing"));
   return Finish(true,TEXT("four fixed-pose captures complete; inspect images and mesh differences before diagnosing"));
  }
 };
}

void S47SaroWallTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47SaroWallTest")))return;
 auto Run=MakeShared<FRun>();Run->Main=Main;FString User;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallFixture="),Run->Fixture);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallOutput="),Run->Output);
 if(!User.StartsWith(TEXT("/tmp/S47-saro-wall-"))||!Run->Fixture.StartsWith(FString(Base)+TEXT("fixtures/"))||!Run->Output.StartsWith(FString(Base)+TEXT("runs/")))
 {Run->Output.Empty();Run->Finish(false,TEXT("requires isolated UserDir and explicit private fixture/output paths"));return;}
 FParse::Value(FCommandLine::Get(),TEXT("S47WallX="),Run->X);FParse::Value(FCommandLine::Get(),TEXT("S47WallZ="),Run->Z);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallYaw="),Run->Yaw);FParse::Value(FCommandLine::Get(),TEXT("S47WallPitch="),Run->Pitch);
 IFileManager::Get().MakeDirectory(*Run->Output,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}
