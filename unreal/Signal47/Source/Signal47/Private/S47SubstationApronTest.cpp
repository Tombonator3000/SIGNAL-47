#include "S47SubstationApronTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 const TCHAR* Asset=TEXT("/Game/SubstationField/SM_SubstationField_ApronGrade.SM_SubstationField_ApronGrade");
 struct FCamera {const TCHAR* Name;FVector Eye,Target;};
 // Identical in before/fixed runs. Mid-apron framing includes both Bx3.2/6.8 lips;
 // toe is By-14. Camera is an explicit low inspection view, separate from body.
 const FCamera Cameras[]={
  {TEXT("day-edge-lod2"),FVector(7885,.25,520.8),FVector(7885,-1.245,524.2)},
  {TEXT("day-toe-lod2"),FVector(7885,-.35,520.3),FVector(7885,-1.245,522.2)}
 };
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;TWeakObjectPtr<UStaticMeshComponent> Grade;
  bool Done=false,CameraInstalled=false,CaptureView=false,Fixed=false;
  int32 Stage=0,View=0,OriginalForcedLod=0;
  double Began=0,At=0,ProgressAt=0,WalkMetres=0,LastFloorAt=-1;
  FVector Last=FVector::ZeroVector,Destination=FVector::ZeroVector;
  FString Root,MeshIdentity;
  TArray<TSharedPtr<FJsonValue>> Checks,Feet,Frames;
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> PreviousCamera;
  FString Image()const{return Root/(FString(Cameras[View].Name)+TEXT(".png"));}
  void Next(int32 S){Stage=S;At=FPlatformTime::Seconds();}
  void Finish(bool Good,const FString& Why)
  {
   if(Done)return;Done=true;auto* M=Main.Get();if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;if(CameraInstalled)M->Player->CameraOverride=MoveTemp(PreviousCamera);}
   if(auto* C=Grade.Get())C->SetForcedLodModel(OriginalForcedLod);
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("expectedFixed"),Fixed);S47J::Set(O,TEXT("seconds"),Began>0?FPlatformTime::Seconds()-Began:0.);S47J::Set(O,TEXT("walkMetres"),WalkMetres);O->SetArrayField(TEXT("checks"),Checks);O->SetArrayField(TEXT("footSamples"),Feet);O->SetArrayField(TEXT("frames"),Frames);
   S47J::Set(O,TEXT("method"),TEXT("Isolated Freeform. One initial real on-foot DINER service-track placement at7885,520.8; ordinary Walker TestMove/Resolve walks to7885,527 and back without intermediate Place. Finite native Walker FloorY and actual step distance/walkability sampled. Only the adopted ApronGrade component is queried; three asset LOD triangle counts, actual visibility/material/transform/NoCollision recorded. Two identical calibrated CameraOverride08 inspection views force existing visual LOD2 in baseline and fixed runs."));
   S47J::Set(O,TEXT("limits"),TEXT("ApronGrade remains decorative NoCollision. Walker floor samples describe the unchanged analytic/proxy floor, not contact with decorative triangles or proof of rendered gap closure. Root must compare the two matched before/after PNGs. No whole-world/asset scan, GPU readback, Story/service regression, human-input or natural route claim. Expected around20 seconds, hard45-second bound after startup."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 substation apron %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
  }
  bool Check(bool Good,const FString& Text)
  {auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Text);Checks.Add(MakeShared<FJsonValueObject>(O));if(!Good)Finish(false,Text);return Good;}
  void StartWalk(const FVector& Target)
  {auto* P=Main.Get()->Player.Get();Destination=Target;Last=P->Pos;ProgressAt=FPlatformTime::Seconds();P->SetActive(true);P->bTestRun=false;}
  bool Walk()
  {
   auto* M=Main.Get();auto* P=M->Player.Get();const double Now=FPlatformTime::Seconds();const FVector D=Destination-P->Pos;
   const double Dist=FVector2D(D.X,D.Z).Size(),Step=FVector2D(P->Pos.X-Last.X,P->Pos.Z-Last.Z).Size();
   if(!(Step<.8&&FMath::IsFinite(P->FloorY)&&P->Walkable(P->Pos.X,P->Pos.Z))){Check(false,TEXT("actual Walker step left finite/bounded/ordinary walkable ground"));return false;}
   WalkMetres+=Step;if(Step>.005)ProgressAt=Now;Last=P->Pos;
   if(Now-LastFloorAt>.2)
   {
    LastFloorAt=Now;auto O=S47J::New();S47J::Set(O,TEXT("phase"),Stage==2?TEXT("outbound"):TEXT("return"));S47J::Set(O,TEXT("x"),P->Pos.X);S47J::Set(O,TEXT("z"),P->Pos.Z);S47J::Set(O,TEXT("nativeFloorY"),P->FloorY);S47J::Set(O,TEXT("actualWalkable"),P->Walkable(P->Pos.X,P->Pos.Z));S47J::Set(O,TEXT("stepMetres"),Step);S47J::Set(O,TEXT("walkedMetres"),WalkMetres);Feet.Add(MakeShared<FJsonValueObject>(O));
   }
   if(Dist<.12){P->TestMove.Reset();return true;}
   if(Now-ProgressAt>5){Finish(false,TEXT("ordinary service-track walk blocked before requested goal"));return false;}
   P->Yaw=FMath::Atan2(-D.X,-D.Z);P->Pitch=-.08;P->TestMove=FVector2D(0,-FMath::Min(1.,Dist*2));return false;
  }
  bool AuditFrame()
  {
   auto* M=Main.Get();auto* C=Grade.Get();auto O=S47J::New();const FVector Eye=S47Three::ToThree(M->Player->EyeUE()),Forward=S47Three::ToThree(M->Player->ForwardUE()*100.);
   if(!Check(C&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden()&&M->Web->ObjAt(M->Web->NodeOf(C)).Shown(),TEXT("exact adopted ApronGrade is actually rendered in current foot area")))return false;
   if(!Check(Eye.Equals(Cameras[View].Eye,.025),TEXT("native camera reached the exact matched inspection eye")))return false;
   if(!Check(MeshIdentity==C->GetStaticMesh()->GetPathName()&&C->ForcedLodModel==3&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("same owned mesh, forced visualLOD2 and decorative NoCollision retained")))return false;
   const auto T=C->GetComponentTransform();const FVector P=T.GetLocation(),S=T.GetScale3D(),Origin=S47Three::ToThree(C->Bounds.Origin);const FQuat Q=T.GetRotation();
   S47J::Set(O,TEXT("image"),Image());S47J::Set(O,TEXT("component"),C->GetPathName());S47J::Set(O,TEXT("node"),M->Web->NodeOf(C));S47J::Set(O,TEXT("mesh"),MeshIdentity);S47J::Set(O,TEXT("area"),M->World->Area);S47J::Set(O,TEXT("clock"),M->Freeform->Clock());S47J::Set(O,TEXT("quality"),M->Flow->Quality);S47J::Set(O,TEXT("effectsLevel"),M->Flow->Graphics.Effects);S47J::Set(O,TEXT("forcedLodModel"),C->ForcedLodModel);S47J::Set(O,TEXT("NoCollision"),true);
   S47J::Set(O,TEXT("actualCameraEyeThree"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(O,TEXT("actualCameraForwardThree"),TArray<double>{Forward.X,Forward.Y,Forward.Z});S47J::Set(O,TEXT("componentTranslationUE"),TArray<double>{P.X,P.Y,P.Z});S47J::Set(O,TEXT("componentRotationQuaternionUE"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});S47J::Set(O,TEXT("componentScaleUE"),TArray<double>{S.X,S.Y,S.Z});S47J::Set(O,TEXT("actualBoundsOriginThree"),TArray<double>{Origin.X,Origin.Y,Origin.Z});
   TArray<double> Lods;for(int32 L=0;L<C->GetStaticMesh()->GetNumLODs();++L)Lods.Add(C->GetStaticMesh()->GetNumTriangles(L));S47J::Set(O,TEXT("actualLodTriangles"),Lods);
   TArray<FString> Materials;for(int32 L=0;L<C->GetNumMaterials();++L)Materials.Add(GetPathNameSafe(C->GetMaterial(L)));S47J::Set(O,TEXT("actualMaterials"),Materials);Frames.Add(MakeShared<FJsonValueObject>(O));return true;
  }
  void Update(AS47Web* W,const TSharedPtr<FRun>& Self)
  {
   if(Done)return;const double Now=FPlatformTime::Seconds();if(Began>0&&Now-Began>45){Finish(false,TEXT("bounded45-second apron test timeout"));return;}
   auto* M=US47Main::Get(W);if(!M||!M->Flow||!M->World||!M->Player||!M->Freeform||!W->IsLoaded()||M->Flow->bStarting||S47Startup::Blocking())return;Main=M;
   if(Stage==0){Began=Now;US47Flow::FStart S;S.bDev=true;S.bFreeform=true;M->Flow->StartGame(S);Next(1);return;}
   if(!M->Freeform->Active()||M->World->bBusy)return;
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Flow->bDevRun,TEXT("fresh isolated Freeform developer case0")))return;
    if(!Check(M->Flow->Quality==TEXT("low")&&M->Flow->Graphics.Effects==0,TEXT("required CLI -S47Quality=low fixes the same actual Low profile in both captures")))return;
    M->UI->Close(true);M->World->Enter(TEXT("diner"));const auto Obj=W->Obj(TEXT("world.legs.road.area.group")).Find(TEXT("substationField:SM_SubstationField_ApronGrade"));
    const auto* Node=Obj?W->Node(Obj.I):nullptr;auto* C=Node?Cast<UStaticMeshComponent>(Node->Comp.Get()):nullptr;
    if(!Check(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==Asset,TEXT("exact active ApronGrade component resolves without a world scan")))return;
    const int32 Old[]={3522,1166,411},New[]={3588,1217,454};bool Counts=C->GetStaticMesh()->GetNumLODs()==3;
    for(int32 L=0;Counts&&L<3;++L)Counts=C->GetStaticMesh()->GetNumTriangles(L)==(Fixed?New[L]:Old[L]);
    if(!Check(Counts,TEXT("all three installed ApronGrade LOD counts match the explicitly requested baseline/fixed contract")))return;
    Grade=C;MeshIdentity=C->GetStaticMesh()->GetPathName();OriginalForcedLod=C->ForcedLodModel;C->SetForcedLodModel(3);
    M->Player->Place(7885,520.8,PI);M->Player->bHeadMotion=false;
    if(!Check(M->Player->Walkable(7885,520.8),TEXT("existing DINER service-track start passes native Walkable")))return;
    PreviousCamera=M->Player->CameraOverride;CameraInstalled=true;const TWeakPtr<FRun> Weak=Self;
    M->Player->CameraOverride=[Weak](double,const FVector2D&,const FVector2D&,FS47View& Out)
    {
     const auto S=Weak.Pin();if(!S||S->Done||!S->CaptureView)return false;const auto& V=Cameras[S->View];const FVector D=V.Target-V.Eye;
     Out.Eye=V.Eye;Out.Yaw=FMath::Atan2(-D.X,-D.Z);Out.Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
    };
    StartWalk(FVector(7885,0,527));Next(2);return;
   }
   if(Stage==2)
   {
    if(!Walk())return;if(!Check(FVector2D(M->Player->Pos.X-7885,M->Player->Pos.Z-527).Size()<.13&&WalkMetres>5.8,TEXT("ordinary Walker reaches apron before the closed fence without intermediate placement")))return;
    StartWalk(FVector(7885,0,520.8));Next(3);return;
   }
   if(Stage==3)
   {
    if(!Walk())return;if(!Check(WalkMetres>11.6&&FVector2D(M->Player->Pos.X-7885,M->Player->Pos.Z-520.8).Size()<.13,TEXT("ordinary Walker returns along the same apron route with measured distance")))return;
    CaptureView=true;M->Player->SetActive(false);Next(4);return;
   }
   if(Now-At<1.5)return;
   if(Stage==4){if(!AuditFrame())return;FScreenshotRequest::RequestScreenshot(Image(),true,false);Next(5);return;}
   TArray<uint8> PNG;if(!FFileHelper::LoadFileToArray(PNG,*Image())||PNG.Num()<1024){if(Now-At>6)Finish(false,TEXT("native matched PNG unavailable"));return;}
   if(!Check(PNG[0]==137&&PNG[1]==80&&PNG[2]==78&&PNG[3]==71,TEXT("native matched inspection PNG saved")))return;
   if(++View<2){Next(4);return;}Finish(true,TEXT("actual apron foot route and two matched forcedLOD2 inspection images completed; visual gap acceptance remains root QA"));
  }
 };
}
void S47SubstationApronTest::Update(AS47Web* Web)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47SubstationApronTest")))return;
 static TSharedPtr<FRun> Run;if(!Run)
 {
  Run=MakeShared<FRun>();Run->Fixed=FParse::Param(FCommandLine::Get(),TEXT("S47ApronExpectedFixed"));FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/S47-substation-apron-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
  Run->Root=FPaths::ProjectSavedDir()/TEXT("SubstationApronTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 }
 Run->Update(Web,Run);
}
