#include "S47FreeformTest.h"
#include "S47FreeformAudioDesk.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "SS47ScreenView.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "S47Props.h"
#include "S47Chapter1.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Spine.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Startup.h"
#include "S47Night.h"
#include "S47Album.h"
#include "S47Image.h"
#include "Misc/Base64.h"
#include "Engine/World.h"
#include "S47FieldCamera.h"
#include "S47Glimpses.h"
#include "S47Flashlight.h"
#include "Components/PrimitiveComponent.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 bool PhotoButtonText(const TSharedRef<SWidget>& W, const FString& Label)
 {
  if (!W->GetVisibility().IsVisible()) return false;
  if (W->GetTypeAsString() == TEXT("STextBlock") && StaticCastSharedRef<STextBlock>(W)->GetText().ToString() == Label) return true;
  auto* Children = W->GetChildren();
  for (int32 I = 0; Children && I < Children->Num(); ++I) if (PhotoButtonText(Children->GetChildAt(I), Label)) return true;
  return false;
 }
 void PhotoButtons(const TSharedRef<SWidget>& W, const FString& Label, TArray<TSharedRef<SButton>>& Out)
 {
  if (!W->GetVisibility().IsVisible()) return;
  if (W->GetTypeAsString() == TEXT("SButton") && PhotoButtonText(W, Label)) Out.AddUnique(StaticCastSharedRef<SButton>(W));
  auto* Children = W->GetChildren();
  for (int32 I = 0; Children && I < Children->Num(); ++I) PhotoButtons(Children->GetChildAt(I), Label, Out);
 }
}

bool FS47FreeformTest::Check(bool Good,const FString& Text)
{
 ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Text);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),Stage);Evidence.Add(MakeShared<FJsonValueObject>(O));
 UE_LOG(LogTemp,Display,TEXT("S47 Freeform %s stage%d: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Stage,*Text);
 if(!Good)Finish(false,Text);return Good;
}
void FS47FreeformTest::Go(int32 Next,double Delay){Stage=Next;At=Age;Hold=Delay;}
void FS47FreeformTest::Shot(const FString& Name){FScreenshotRequest::RequestScreenshot(Root/(Name+TEXT(".png")),true,false);}
void FS47FreeformTest::Finish(bool Good,const FString& Why)
{
 if(bDone)return;bDone=true;if(M&&M->Player)M->Player->TestMove.Reset();
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("diskFlowRestores"),Restores);S47J::Set(O,TEXT("scope"),FParse::Param(FCommandLine::Get(),TEXT("S47FieldPlanTest"))?TEXT("field-plan-disk-continue"):FParse::Param(FCommandLine::Get(),TEXT("S47ArrayCapacityTest"))?TEXT("array-capacity-controlled-fixture"):FParse::Param(FCommandLine::Get(),TEXT("S47GuidanceTest"))?TEXT("guidance-state-fixtures"):FParse::Param(FCommandLine::Get(),TEXT("S47OpsNavigationTest"))?TEXT("ops-native-navigation"):FParse::Param(FCommandLine::Get(),TEXT("S47ShopBasketTest"))?TEXT("shop-multivare-native-fixture"):FParse::Param(FCommandLine::Get(),TEXT("S47CameraPickupTest"))?TEXT("camera-physical-pickup"):FParse::Param(FCommandLine::Get(),TEXT("S47PrinterTest"))?TEXT("printer-native-fixture"):FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTripHoldTest"))?TEXT("actual-trip-work-hold"):FParse::Param(FCommandLine::Get(),TEXT("S47PhotoEconomyTest"))?TEXT("photo-economy-rendered-fixture"):bFieldResume?TEXT("field-and-events"):TEXT("full"));O->SetArrayField(TEXT("evidence"),Evidence);
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PrinterLegacyQueueTest")))S47J::Set(O,TEXT("scope"),TEXT("legacy-printer-queue-regression"));
 if(!PhotoEvent.IsEmpty())S47J::Set(O,TEXT("photoFixtureEvent"),PhotoEvent);
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PhotoEconomyTest")))
  S47J::Set(O,TEXT("method"),PhotoEvent==TEXT("feeder_presence")?TEXT("Photo-only native fixture: explicit nighttime clock, road placement and Force encounter; rendered ControlledPhotoBody, real camera and separate live flashlight-OFF/ON exposures. Actor and camera may move; this is not identical-pose lighting A/B. A qualifying ON exposure and one incident claim do not imply that ON created the claim: OFF may already qualify. Production OPS registration plus unpaid/paid disk Flow restores verify payment persistence. No natural scheduling, acquisition/audio journey or OS-input beta is established."):TEXT("Photo-only native fixture: Force landing trace, explicit camera placement, real rendered exposure, duplicate guard, physical OPS registration and unpaid/paid disk Flow restores. No natural scheduling, acquisition/audio journey or OS-input beta is established."));
 else
  S47J::Set(O,TEXT("method"),TEXT("Isolated native Freeform integration: actual timed encoder settling, acquisition and audio; production workstation callbacks, full disk/Flow restores, accelerated day transitions and explicit event fixtures. Screenshots are native Vulkan. Fixture placement does not establish a natural town drive or human input/listening."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTripHoldTest")))
 {
  S47J::Set(O,TEXT("method"),TEXT("Explicit receiver tuning and one array target command; real generated pulsar job, timed acquisition, retained original, internal control, Roam/StartTrip busy callback and ordinary stationary exit. Frame samples compare native encoders, control progress and world clock through the actual transition; no bBusy write, natural commute or OS-input claim."));
  S47J::Set(O,TEXT("busy_frame_samples"),TripHoldSamples);S47J::Set(O,TEXT("trip_hold_baseline"),Snapshot);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PrinterTest")))S47J::Set(O,TEXT("method"),TEXT("Explicit prepared-job fixtures, real timed printer feed, native paper transform and audio-loop state, disk/Flow Continue, power interruption and production collection. No natural acquisition or subjective listening acceptance."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47CameraPickupTest")))S47J::Set(O,TEXT("method"),TEXT("Fresh unowned camera, physical photolab model and actual interaction ray/use, production toggle/shutter, two disk/Flow restores, explicit legacy-field migration and Story placement smoke. Fixture player placement, not a natural walk or full Story regression."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47ShopBasketTest")))S47J::Set(O,TEXT("method"),TEXT("Native store menu clicks, one paid multi-item parcel, production pickup/load/unload/receiving, disk/Flow Continue and legacy singleton fixture. Clock/player placement and truck relocation are explicit fixtures; no natural drive, walk or credit-policy regression claimed."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47OpsNavigationTest")))S47J::Set(O,TEXT("method"),TEXT("Native on-glass OPS buttons across all six pages, daily job selection, empty archive guidance, service accept/focus and a separate physical screen leave. Fixture screen approach; no OS-input or full work-order completion claimed."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47FieldPlanTest")))S47J::Set(O,TEXT("method"),TEXT("Explicit roof_steps case lead and production PlanCaseTest(2,1), followed by real disk/Flow Continue. Tests retained pending plan only; no enacted world test, field evidence or reward claimed."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47ArrayCapacityTest")))S47J::Set(O,TEXT("method"),TEXT("Explicit settled source-2 fixtures measure ten production Step(.1) calls at 27/24/23/repaired27 capacity; actual S-03 service fault persists through disk/Flow Continue. Receiver screenshot at degraded capacity; no natural acquisition or motor trajectory claim."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47GuidanceTest")))S47J::Set(O,TEXT("method"),TEXT("Representative explicit job-state fixtures exercise production location/action hints; one selected clock-comparison state survives disk/Flow Continue. No full observing journey or automatic completion claimed."));
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PrinterLegacyQueueTest")))S47J::Set(O,TEXT("method"),TEXT("Explicit pre-3b6a6c1c original-document format and missing legacy fields; real disk/Flow Continue, timed subsequent sheet and production Use(printer). Expected FAIL until queue migration is fixed; no natural acquisition claimed."));
 S47J::Set(O,TEXT("limits"),TEXT("No human three-day pacing acceptance, subjective horror/listening, full-story regression or recipient hardware acceptance."));
 FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
 FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
bool FS47FreeformTest::Restore(const FString& Name)
{
 const auto O=M->Flow->GameState();const FString File=Root/(Name+TEXT(".json"));
 if(!Check(FFileHelper::SaveStringToFile(S47J::Write(O),*File),TEXT("full state written to isolated disk: ")+Name))return false;
 FString Text;if(!Check(FFileHelper::LoadFileToString(Text,*File),TEXT("fresh disk state read: ")+Name))return false;
 const auto State=S47J::Read(Text);if(!Check(State&&S47J::Str(State,TEXT("sessionMode"))==TEXT("freeform"),TEXT("saved mode identity retained: ")+Name))return false;
 US47Flow::FStart S;S.bDev=true;S.State=State;M->Flow->StartGame(S);++Restores;
 if(!Check(M->IsFreeform()&&M->Freeform->Active(),TEXT("ordinary Flow restore starts Freeform: ")+Name))return false;
 const auto P=S47J::Obj(State,TEXT("truckPose"));
 if(P&&M->World)
 {
  const FVector Expected(S47J::Num(P,TEXT("x")),S47J::Num(P,TEXT("y")),S47J::Num(P,TEXT("z")));
  if(!Check(M->World->ServicePlace()==S47J::Str(State,TEXT("truck"))&&M->World->ServicePosition().Equals(Expected,.01),TEXT("parked service vehicle identity and actual pose survive Continue: ")+Name))return false;
 }
 return true;
}
bool FS47FreeformTest::FileAtBinder()
{
 if(!Check(M&&M->Web&&M->Web->Item(TEXT("binder")),TEXT("physical run binder exists in the loaded SARO world")))return false;
 const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("binder")));M->UI->Close(true);M->Player->Place(P.X,P.Z-.8,0);
 return Check(M->Freeform->FilePacket(),TEXT("retained packet receives filing credit at the actual physical run binder"));
}
void FS47FreeformTest::PoseForEvent()
{
 const FString Id=M->Freeform->Director->ActiveEvent();
  if(Id==TEXT("response_lights")||Id==TEXT("flyover")||Id==TEXT("close_pass")||Id==TEXT("empty_car"))
  {
   const bool Car=Id==TEXT("empty_car");const FVector A=M->Freeform->Director->EventAnchor();
   M->Player->Place(Car?A.X-4.:A.X,Car?A.Z+5.:A.Z+18.,0);
   const FVector D=A-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
}
bool FS47FreeformTest::PosePhotoFigure()
{
 if(!M||!M->Night||!M->Night->Glimpses)return false;
 for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered&&M->Night->Glimpses->ControlledPhotoBody(I.Id))
 {
  PhotoFigureId=I.Id;const FVector P=I.Eye-FVector(0,.5,0);
  // Explicit fixture placement only; the body, lighting and camera capture remain production systems.
  M->Player->Place(I.Pos.X,I.Pos.Z+5.,0);
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
 }
 return false;
}
void FS47FreeformTest::LogPhotoFrame(const FString& Label,const FS47AlbumShot& Photo)
{
 auto O=S47J::New();S47J::Set(O,TEXT("photoFixtureFrame"),Label);S47J::Set(O,TEXT("event"),PhotoEvent);S47J::Set(O,TEXT("clock"),M->Freeform->Clock());
 S47J::Set(O,TEXT("fullFrameMean"),M->Night->Album->Last?M->Night->Album->Last->Mean:-1.);
 S47J::Set(O,TEXT("subjectMean"),M->Freeform->Director->LastPhotoSubjectMean());
 S47J::Set(O,TEXT("lampOn"),M->Night->Flashlight->bOn);S47J::Set(O,TEXT("lampIntensity"),M->Night->Flashlight->Intensity());
 S47J::Set(O,TEXT("claims"),M->Freeform->Director->PhotoClaims().Num());S47J::Set(O,TEXT("figureId"),PhotoFigureId);
 S47J::Set(O,TEXT("activeEvent"),M->Freeform->Director->ActiveEvent());S47J::Set(O,TEXT("area"),M->Web->Area());S47J::Set(O,TEXT("shotId"),Photo.Id);S47J::Set(O,TEXT("targetCount"),Photo.Targets.Num());
 TArray<FColor> Pixels;int32 W=0,H=0;const bool Decoded=S47Image::DecodePixels(Photo.Url,Pixels,W,H);bool Written=false;
 const FString ImagePath=Root/(TEXT("photo-original-")+Label+TEXT(".png"));
 if(Decoded)
 {
  const FString Url=S47Image::ToDataUrl(Pixels,W,H,false);int32 Comma=INDEX_NONE;TArray<uint8> Bytes;
  Written=Url.FindChar(TCHAR(','),Comma)&&FBase64::Decode(Url.Mid(Comma+1),Bytes)&&FFileHelper::SaveArrayToFile(Bytes,*ImagePath);
 }
 S47J::Set(O,TEXT("originalDecoded"),Decoded);S47J::Set(O,TEXT("originalPngWritten"),Written);S47J::Set(O,TEXT("originalWidth"),W);S47J::Set(O,TEXT("originalHeight"),H);S47J::Set(O,TEXT("originalPng"),ImagePath);

 bool Rendered=false;
 for(const auto& I:M->Night->Glimpses->Individuals())if(I.Id==PhotoFigureId)
 {
  const auto* Body=M->Night->Glimpses->ControlledPhotoBody(I.Id);Rendered=I.bRendered&&Body&&Body->IsVisible();
  const auto P=M->FieldCamera->Project(I.Eye-FVector(0,.5,0)),Top=M->FieldCamera->Project(I.Eye+FVector(0,.1,0)),Bottom=M->FieldCamera->Project(I.Eye-FVector(0,1.1,0));
  const FVector RayFrom=M->FieldCamera->PhotoLoc,RayTo=S47Three::ToUE(I.Eye-FVector(0,.5,0));
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformPhotoFixtureSight),true);Params.AddIgnoredActor(M->Player);
  const bool Blocked=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,RayFrom,RayTo,ECC_Visibility,Params);
  S47J::Set(O,TEXT("rayFromUE"),RayFrom.ToString());S47J::Set(O,TEXT("rayToUE"),RayTo.ToString());S47J::Set(O,TEXT("rayBlocked"),Blocked);
  S47J::Set(O,TEXT("firstHitActor"),Hit.GetActor()?Hit.GetActor()->GetPathName():FString());S47J::Set(O,TEXT("firstHitComponent"),Hit.Component.IsValid()?Hit.Component->GetPathName():FString());
  S47J::Set(O,TEXT("bodyComponent"),Body?Body->GetPathName():FString());S47J::Set(O,TEXT("firstHitIsBody"),Blocked&&Hit.Component.Get()==Body);
  S47J::Set(O,TEXT("hitDistance"),Blocked?double(Hit.Distance):-1.);S47J::Set(O,TEXT("startPenetrating"),Hit.bStartPenetrating);S47J::Set(O,TEXT("impactPointUE"),Hit.ImpactPoint.ToString());
  if(Body)
  {
   S47J::Set(O,TEXT("bodyCollision"),int32(Body->GetCollisionEnabled()));S47J::Set(O,TEXT("bodyVisibilityResponse"),int32(Body->GetCollisionResponseToChannel(ECC_Visibility)));
   const FBox Box=Body->Bounds.GetBox();FBox2D Bounds(ForceInit);bool Front=true;
   for(int32 C=0;C<8;++C)
   {
    const FVector Corner((C&1)?Box.Max.X:Box.Min.X,(C&2)?Box.Max.Y:Box.Min.Y,(C&4)?Box.Max.Z:Box.Min.Z);
    const auto Q=M->FieldCamera->Project(S47Three::ToThree(Corner));Front&=Q.bFront;Bounds+=FVector2D(Q.U,Q.V);
   }
   S47J::Set(O,TEXT("boundsAllInFront"),Front);S47J::Set(O,TEXT("boundsMinUV"),Bounds.Min.ToString());S47J::Set(O,TEXT("boundsMaxUV"),Bounds.Max.ToString());
  }

  S47J::Set(O,TEXT("u"),P.U);S47J::Set(O,TEXT("v"),P.V);S47J::Set(O,TEXT("inFront"),P.bFront);S47J::Set(O,TEXT("topV"),Top.V);S47J::Set(O,TEXT("bottomV"),Bottom.V);S47J::Set(O,TEXT("frameHeight"),FMath::Abs(Bottom.V-Top.V));break;
 }
 S47J::Set(O,TEXT("renderedControlledBody"),Rendered);Evidence.Add(MakeShared<FJsonValueObject>(O));
 UE_LOG(LogTemp,Display,TEXT("S47 photo fixture: %s"),*S47J::Write(O));
}
void FS47FreeformTest::Step(double Dt)
{
 if(bDone)return;
 if(!bInit)
 {
  bInit=true;if(!FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTest"))){bDone=true;return;}
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  Root=FPaths::ProjectSavedDir()/TEXT("FreeformTest");IFileManager::Get().MakeDirectory(*Root,true);
  if(!Check(User.StartsWith(TEXT("/tmp/")),TEXT("explicit isolated temporary profile required")))return;
 }
 if(Stage==8690&&FParse::Param(FCommandLine::Get(),TEXT("S47CameraPickupTest")))
 {
  if(!M||!M->Web||!M->Web->IsLoaded()||!M->Flow||M->Flow->bStarting||S47Startup::Blocking())return;
  const auto Camera=M->Web->Obj(TEXT("room.fieldCamera"));
  if(!Check(!M->IsFreeform()&&Camera&&Camera.WorldPosition().Equals(FVector(5.8,.97,.66),.001),TEXT("Story start restores immutable original camera placement after Freeform relocation")))return;
  Finish(true,TEXT("Physical camera pickup, shutter, ownership Continue, legacy migration and Story placement passed."));return;
 }
 if(!M||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Flow||M->Flow->bStarting||S47Startup::Blocking())return;
 Age+=FMath::Min(Dt,.2);if(Age>900){Finish(false,TEXT("bounded integration timeout"));return;}if(Hold>0){Hold-=Dt;return;}
 auto& F=*M->Freeform;using J=FS47FreeformSession::EJobStage;
 if(FParse::Param(FCommandLine::Get(),TEXT("S47FieldPlanTest")))
 {
  if(Age>40){Finish(false,TEXT("bounded field-plan fixture timeout"));return;}
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.Director,TEXT("isolated explicit field-plan fixture")))return;
   M->UI->Close(true);F.Director->Stop();WorkBudget=F.Budget();JobIndex=F.Jobs().Num();
   F.OpenCaseFor(TEXT("roof_steps"));
   if(!Check(F.PlanCaseTest(2,1)&&F.PlannedFieldTest(2)==1,TEXT("production API opens roof-steps case and plans physical test without enacting it")))return;
   const auto* C=F.CaseFile().Get(2);GuidanceExpected=C?C->PlannedJob:FString();
   if(!Check(C&&!GuidanceExpected.IsEmpty()&&!C->Results[1].bDone&&C->ValidTests==0&&!C->bConcluded&&F.Jobs().Num()==JobIndex&&F.Budget()==WorkBudget,TEXT("pending field plan has its own ID with no receiver job, result or payment")))return;
   if(!Restore(TEXT("pending-field-plan")))return;Go(8961,.3);return;
  }
  if(Stage==8961)
  {
   const auto* C=F.CaseFile().Get(2);
   if(!Check(C&&F.PlannedFieldTest(2)==1&&C->PlannedJob==GuidanceExpected&&!C->Results[0].bDone&&!C->Results[1].bDone&&!C->Results[2].bDone&&C->ValidTests==0&&!C->bConcluded&&F.Jobs().Num()==JobIndex&&F.Budget()==WorkBudget,TEXT("disk Continue preserves exact pending field plan independently of receiver jobs, without results or rewards")))return;
   if(!Check(F.CancelFieldTest(2)&&F.PlannedFieldTest(2)==-1&&!F.CancelFieldTest(2),TEXT("pending field plan can be cancelled once after Continue without stranding the player")))return;
   C=F.CaseFile().Get(2);
   if(!Check(C&&C->PlannedJob.IsEmpty()&&!C->Results[0].bDone&&!C->Results[1].bDone&&!C->Results[2].bDone&&C->ValidTests==0&&!C->bConcluded&&F.Jobs().Num()==JobIndex&&F.Budget()==WorkBudget,TEXT("field-plan cancellation grants no result, receiver job or payment")))return;
   Finish(true,TEXT("Pending field-plan disk Continue and cancellation passed; physical field-test enactment remains outside this fixture."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47ArrayCapacityTest")))
 {
  if(Age>45){Finish(false,TEXT("bounded array-capacity fixture timeout"));return;}
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.Field&&F.Director&&M->Web->DishCount()==27&&F.HasPowerForReactiveLight(),TEXT("isolated powered 27-dish fixture, without natural observing journey claim")))return;
   M->UI->Close(true);F.Director->Stop();
   if(!Check(F.SelectJob(2)&&F.ChooseThird(false),TEXT("explicit third-job hydrogen-drift choice selects source 2 before fixture snapshot")))return;
   Snapshot=S47J::Read(S47J::Write(F.Save()));int32 SourceIndex=INDEX_NONE;
   const auto Jobs=S47J::Arr(Snapshot,TEXT("jobs"));for(int32 I=0;I<Jobs.Num();++I)if(S47J::Int(Jobs[I]->AsObject(),TEXT("source"))==2){SourceIndex=I;break;}
   if(!Check(SourceIndex!=INDEX_NONE,TEXT("existing source-2 card available for controlled capacity fixture")))return;
   S47J::Set(Snapshot,TEXT("selected"),SourceIndex);S47J::Set(Snapshot,TEXT("settle"),1.);S47J::Set(Snapshot,TEXT("crossRunning"),false);Snapshot->RemoveField(TEXT("angles"));
   auto Job=Jobs[SourceIndex]->AsObject();S47J::Set(Job,TEXT("stage"),int32(J::Integrating));S47J::Set(Job,TEXT("elapsed"),0.);
   // Each case starts with the same retained card, settled encoders and tuning.
   // Ten production Step(.1) calls measure one controlled second, without oversize Dt.
   for(const int32 Count:{27,24,23,27})
   {
    F.Begin(S47J::Read(S47J::Write(Snapshot)));F.Director->Stop();
    const auto P=F.ConsoleProfile();if(!Check(P!=nullptr&&F.CurrentJob()->bGenerated,TEXT("capacity fixture retains actual receiver card")))return;
    const double Az=(P->Az[0]+P->Az[1])/2,El=F.CurrentJob()->Card.El;
    for(int32 I=0;I<27;++I){M->Web->SetDishRemoteAvailable(I,I<Count);M->Web->SnapDish(I,Az,El);M->Web->PointDish(I,Az,El,0,true);}
    auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
    for(int32 Tick=0;Tick<11;++Tick)F.Step(.1); // production one-second settling before the equal measurement windows
    const double Before=F.CurrentJob()->Elapsed;for(int32 Tick=0;Tick<10;++Tick)F.Step(.1);
    const double Expected=Count<24?0.:double(Count*(Count-1))/(27.*26.);
    if(!Check(F.AvailableDishCount()==Count&&FMath::IsNearlyEqual(F.ArrayMeasurementRate(),Expected,1e-8)&&FMath::IsNearlyEqual(F.CurrentJob()->Elapsed-Before,Expected,1e-6),FString::Printf(TEXT("controlled one-second source-2 sampling at %d dishes: measured %.6f expected %.6f"),Count,F.CurrentJob()->Elapsed-Before,Expected)))return;
    if(Count==23&&!Check(F.NextAction().Contains(TEXT("BLOCKED"))&&F.NextAction().Contains(TEXT("ARRAY STATUS"))&&F.JobLocation(*F.CurrentJob()).Contains(TEXT("ARRAY STATUS")),TEXT("blocked acquisition directs to physical array-status repair workstation")))return;
   }
   for(int32 I=0;I<27;++I)M->Web->SetDishRemoteAvailable(I,I<24);
   F.OpenReceiver();Go(8951,1.2);return;
  }
  if(Stage==8951)
  {
   if(!Check(F.TerminalOnGlass()&&F.TerminalWindowRendered(),TEXT("degraded receiver renders on the physical CRT")))return;Shot(TEXT("array-24-degraded-receiver"));Go(89511,.2);return;
  }
  if(Stage==89511){F.OpenTracking();Go(89512,1.2);return;}
  if(Stage==89512){Shot(TEXT("array-24-degraded-tracking"));Go(8952,.2);return;}
  if(Stage==8952)
  {
   M->UI->Close(true);for(int32 I=0;I<27;++I)M->Web->SetDishRemoteAvailable(I,true);
   if(!Check(F.Field->TriggerServiceFault()&&F.AvailableDishCount()==26,TEXT("actual S-03 service fault isolates one antenna before disk save")))return;
   if(!Restore(TEXT("array-s03-service-fault")))return;Go(8953,.3);return;
  }
  if(Stage==8953)
  {
   if(!Check(F.Field->HasArrayFault()&&F.AvailableDishCount()==26&&FMath::IsNearlyEqual(F.ArrayMeasurementRate(),25./27.,1e-8),TEXT("disk Continue derives 26-dish capacity from persisted S-03 service state, without saving rate or generic test isolation")))return;
   Finish(true,TEXT("Controlled 27/24/23/repaired sampling and actual S-03 disk Continue passed."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47GuidanceTest")))
 {
  if(Age>45){Finish(false,TEXT("bounded guidance fixture timeout"));return;}
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.CurrentJob(),TEXT("isolated guidance fixture has a real generated work card")))return;
   M->UI->Close(true);F.Director->Stop();auto Job=*F.CurrentJob();Job.CaseLine=Job.CaseTest=-1;Job.bAnomaly=false;Job.bControl=false;Job.ControlAt=0;
   auto Hint=[&](const TCHAR* Place,const TCHAR* Action,const TCHAR* Location){const FString Text=F.JobNextAction(Job);return Check(Text.Contains(TEXT(":"))&&Text.Contains(Place)&&Text.Contains(Action)&&F.JobLocation(Job).Contains(Location),FString(TEXT("explicit guidance fixture / "))+Text);};
   Job.Stage=J::New;if(!Hint(TEXT("TRACKING"),TEXT("POINT THE ARRAY"),TEXT("Tracking")))return;
   Job.Stage=J::Integrating;if(!Hint(TEXT("RECEIVER"),TEXT("KEEP TUNING"),TEXT("Receiver")))return;
   Job.Stage=J::Chart;Job.ChartFeed=FS47FreeformSession::PrinterSeconds;if(!Hint(TEXT("PRINTER"),TEXT("RETAIN"),TEXT("Printer")))return;
   Job.Stage=J::Recorded;if(!Hint(TEXT("OPS"),TEXT("REGISTER"),TEXT("OPS")))return;
   Job.bAnomaly=true;Job.Phenomenon=TEXT("wrong_source");
   if(!Hint(TEXT("TRACKING"),TEXT("MEASURE WEST"),TEXT("Tracking")))return;
   Job.bControl=true;Job.ControlAt=4;
   if(!Hint(TEXT("TRACKING"),TEXT("WAIT FOR AUTOMATIC"),TEXT("Tracking")))return;
   if(!Check(F.JobNextAction(Job).Contains(TEXT("THEN COMPARE AT OPS")),TEXT("automatic adjacent-beam guidance names the next workstation")))return;
   Job.ControlAt=8;Job.bBeamVerified=true;
   if(!Hint(TEXT("OPS"),TEXT("DAILY WORK"),TEXT("OPS")))return;
   Job.Phenomenon=TEXT("time_disagreement");Job.ControlAt=4;
   if(!Hint(TEXT("RECEIVER RACK"),TEXT("THEN COMPARE CLOCK READINGS AT OPS"),TEXT("Receiver rack")))return;
   Job.ControlAt=8;Job.bClockMeasured=true;if(!Hint(TEXT("OPS"),TEXT("COMPARE THE RETAINED"),TEXT("OPS")))return;
   Job.Phenomenon=TEXT("duplicate_print");Job.ControlAt=4;
   if(!Hint(TEXT("RECEIVER RACK"),TEXT("AUTOMATIC REGISTER CHECK"),TEXT("Receiver rack")))return;
   Job.ControlAt=8;Job.DuplicateFeed=FS47FreeformSession::PrinterSeconds;
   if(!Hint(TEXT("PRINTER"),TEXT("WITHOUT ALTERING THE ORIGINAL"),TEXT("Printer")))return;
   Job.bDuplicateCollected=true;if(!Hint(TEXT("OPS"),TEXT("COMPARE THE RETAINED"),TEXT("OPS")))return;
   Job.Phenomenon=TEXT("extra_pulse");Job.ControlAt=4;
   if(!Hint(TEXT("RECEIVER RACK"),TEXT("THEN REVIEW BOTH RECORDINGS"),TEXT("Receiver rack")))return;
   Job.ControlAt=8;if(!Hint(TEXT("REEL DECK"),TEXT("MARK THE EXTRA TICKS"),TEXT("Reel deck")))return;
   Job.bAudioReviewed=true;if(!Hint(TEXT("OPS"),TEXT("REGISTER THE SUPPORTED"),TEXT("OPS")))return;
   Job.Stage=J::Registered;Job.PacketFeed=FS47FreeformSession::PrinterSeconds;if(!Hint(TEXT("PRINTER"),TEXT("COLLECT"),TEXT("Printer")))return;
   Job.Stage=J::Packet;if(!Hint(TEXT("RECORDS ROOM"),TEXT("BINDER"),TEXT("binder")))return;
   Job.Stage=J::Filed;if(!Check(F.JobNextAction(Job).Contains(TEXT("DAILY WORK OR CASE ARCHIVE")),TEXT("filed work directs to further work or retained archive")))return;
   auto Save=S47J::Read(S47J::Write(F.Save()));auto O=S47J::Arr(Save,TEXT("jobs"))[0]->AsObject();
   S47J::Set(O,TEXT("stage"),int32(J::Recorded));S47J::Set(O,TEXT("anomaly"),true);S47J::Set(O,TEXT("phenomenon"),TEXT("time_disagreement"));S47J::Set(O,TEXT("control"),true);S47J::Set(O,TEXT("controlAt"),8.);S47J::Set(O,TEXT("clockMeasured"),true);S47J::Set(O,TEXT("reviewed"),false);S47J::Set(Save,TEXT("selected"),0);F.Begin(Save);F.Director->Stop();
   GuidanceExpected=F.NextAction();
   if(!Check(GuidanceExpected.StartsWith(TEXT("OPS (corner PC):"))&&GuidanceExpected.Contains(TEXT("COMPARE THE RETAINED")),TEXT("selected completed clock-control fixture offers a concrete OPS comparison action")))return;
   if(!Restore(TEXT("guidance-clock-comparison")))return;Go(8901,.3);return;
  }
  if(Stage==8901)
  {
   if(!Check(F.CurrentJob()&&F.CurrentJob()->Phenomenon==TEXT("time_disagreement")&&F.CurrentJob()->ControlAt==8&&!F.CurrentJob()->bAudioReviewed&&F.NextAction()==GuidanceExpected,TEXT("disk Continue retains selected step and exact place/action hint without completing its comparison")))return;
   Finish(true,TEXT("Representative production guidance and selected-state disk Continue passed."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47OpsNavigationTest")))
 {
  if(Age>60){Finish(false,TEXT("bounded OPS navigation timeout"));return;}
  auto Roots=[&](){TArray<TSharedRef<SWidget>> Out;TSet<const SWidget*> Seen;
   if(F.TerminalOnGlass()&&FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& User){TSharedPtr<SWidget> W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;if(W->GetTypeAsString().Contains(TEXT("Window"))&&!Seen.Contains(W.Get())){Seen.Add(W.Get());Out.Add(W.ToSharedRef());}},true);return Out;};
  auto Click=[&](const FString& Label){TArray<TSharedRef<SButton>> Buttons;for(const auto& W:Roots())PhotoButtons(W,Label,Buttons);
   if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("unique enabled OPS native button: ")+Label))return false;
#if !UE_BUILD_SHIPPING
   Buttons[0]->SimulateClick();return true;
#else
   Finish(false,TEXT("native OPS navigation requires development build"));return false;
#endif
  };
  TFunction<bool(const TSharedRef<SWidget>&,const FString&)> Contains;
  Contains=[&](const TSharedRef<SWidget>& W,const FString& Text){if(!W->GetVisibility().IsVisible())return false;if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(Text))return true;auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)if(Contains(C->GetChildAt(I),Text))return true;return false;};
  auto HasText=[&](const FString& Text){for(const auto& W:Roots())if(Contains(W,Text))return true;return false;};
  auto StillAtOps=[&](){return F.TerminalOnGlass()&&F.ActiveTerminalScreen()==3&&M->Hud->ModalWidget()==OpsModal.Pin()&&M->UI->Modal();};
  static const TCHAR* Labels[]={TEXT("DAILY WORK"),TEXT("CASE BOARD"),TEXT("CASE ARCHIVE"),TEXT("SERVICE"),TEXT("SUPPLIES")};
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&M->World->Area==TEXT("saro")&&F.Jobs().Num()>=2,TEXT("isolated fresh SARO OPS navigation fixture")))return;
   M->UI->Close(true);F.Director->Stop();const FVector Ops=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(Ops.X,Ops.Z+1.1,0);F.Use(TEXT("opsTerminal"));OpsModal=M->Hud->ModalWidget();OpsNavStep=0;Go(8801,.8);return;
  }
  if(Stage==8801)
  {
   if(!Check(StillAtOps()&&F.TerminalWindowRendered()&&F.OperationsSection()==0&&HasText(TEXT("MORE WORK /")),TEXT("native OPS overview explains more work while rendering on the physical CRT")))return;
   Shot(TEXT("ops-overview"));Go(8802,.3);return;
  }
  if(Stage==8802)
  {
   if(!Click(Labels[OpsNavStep]))return;Go(8803,.3);return;
  }
  if(Stage==8803)
  {
   if(!Check(StillAtOps()&&F.OperationsSection()==OpsNavStep+1,FString(TEXT("OPS navigation changes page without closing or replacing the physical screen: "))+Labels[OpsNavStep]))return;
   if(OpsNavStep==0)
   {
    if(!Check(HasText(TEXT("MORE WORK /")),TEXT("daily work explains available and carried-over jobs")))return;
    const FString Id=F.Jobs()[1].Id;if(!Click(Id))return;
    if(!Check(StillAtOps()&&F.CurrentJob()->Id==Id&&F.OperationsSection()==1,TEXT("native daily-work row selects another available job inside OPS")))return;
    Shot(TEXT("ops-daily-work"));Go(8804,.3);return;
   }
   if(OpsNavStep==2)
   {
    if(!Check(HasText(TEXT("NO RETAINED RECORDS YET"))&&HasText(TEXT("collect its original at the printer")),TEXT("empty archive explains where retained records come from")))return;
    Shot(TEXT("ops-empty-archive"));Go(8804,.3);return;
   }
   if(OpsNavStep==3)
   {
    if(!Click(TEXT("Accept / use SARO stock")))return;Go(88031,.3);return;
   }
   Go(8804);return;
  }
  if(Stage==88031)
  {
   if(!Check(StillAtOps()&&F.OperationsSection()==4&&F.Field->HasArrayFault(),TEXT("accepting service order refreshes SERVICE while the same OPS screen stays open")))return;
   if(!Click(TEXT("Show observing objective / retain unfinished service")))return;Go(88032,.3);return;
  }
  if(Stage==88032)
  {
   if(!Check(StillAtOps()&&F.OperationsSection()==4&&HasText(TEXT("Follow service objective")),TEXT("service focus toggle remains inside OPS and offers the inverse action")))return;
   Go(8804);return;
  }
  if(Stage==8804)
  {
   if(!Click(TEXT("OVERVIEW")))return;Go(8805,.3);return;
  }
  if(Stage==8805)
  {
   if(!Check(StillAtOps()&&F.OperationsSection()==0,TEXT("OVERVIEW returns home without a physical exit")))return;
   if(++OpsNavStep<5){Go(8802);return;}
   // Explicit completed-work/retained-original fixture for the two alternate UI states.
   auto Save=S47J::Read(S47J::Write(F.Save()));auto Jobs=S47J::Arr(Save,TEXT("jobs"));
   for(const auto& V:Jobs)S47J::Set(V->AsObject(),TEXT("stage"),int32(J::Filed));
   S47J::Set(Jobs[0]->AsObject(),TEXT("printed"),true);S47J::Set(Jobs[0]->AsObject(),TEXT("originalPage"),TEXT("Explicit OPS test retained original"));
   M->UI->Close(true);F.Begin(Save);F.Director->Stop();F.Use(TEXT("opsTerminal"));OpsModal=M->Hud->ModalWidget();Go(88051,.6);return;
  }
  if(Stage==88051)
  {
   if(!Check(StillAtOps()&&HasText(TEXT("ALL AVAILABLE WORK COMPLETE"))&&HasText(TEXT("REST UNTIL MORNING")),TEXT("explicit completed-work fixture explains how to get the next day's jobs")))return;
   if(!Click(TEXT("CASE ARCHIVE")))return;Go(88052,.3);return;
  }
  if(Stage==88052)
  {
   if(!Check(StillAtOps()&&F.OperationsSection()==3&&!HasText(TEXT("NO RETAINED RECORDS YET"))&&HasText(F.Jobs()[0].Id+TEXT(" / ORIGINAL")),TEXT("retained-original fixture replaces empty archive guidance with the actual indexed job")))return;
   if(!Click(TEXT("OVERVIEW")))return;Go(88053,.3);return;
  }
  if(Stage==88053)
  {
   const auto Modal=M->Hud->ModalWidget();
   if(!Check(Modal&&Modal->GetTypeAsString()==TEXT("SS47ScreenView"),TEXT("physical leave uses the actual screen view controller")))return;
   StaticCastSharedPtr<SS47ScreenView>(Modal)->Leave();Go(8806,1.);return;
  }
  if(Stage==8806)
  {
   if(!Check(!F.TerminalOnGlass()&&!M->UI->Modal()&&F.ActiveTerminalScreen()==-1,TEXT("separate physical leave returns to room and clears OPS modal")))return;
   Finish(true,TEXT("All OPS pages, job selection, empty archive, service accept/focus, overview return and physical leave passed."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47ShopBasketTest")))
 {
  if(Age>120){Finish(false,TEXT("bounded store basket test timeout"));return;}
  auto Contents=[&](){const auto& C=F.Field->ParcelContents();return C.Num()==2&&C.FindRef(TEXT("fuse"))==2&&C.FindRef(TEXT("relay"))==1;};
  auto StockUnchanged=[&](){return F.InventoryCount(TEXT("fuse"))==ShopFuses&&F.InventoryCount(TEXT("relay"))==ShopRelays;};
  auto Place=[&](const FVector& P){M->Player->Place(P.X,P.Z,0);};
  auto Aim=[&](const FVector& Target){const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);const FVector D=Target-Eye;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());};
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.Field&&F.Field->ParcelStage()==0&&F.Budget()>=35,TEXT("isolated store fixture starts with funds and no parcel")))return;
   M->UI->Close(true);F.Director->Stop();const double Hour=FMath::Fmod(F.Clock(),86400.);
   if(!Check(F.TestAdvance(FMath::Fmod(10*3600.-Hour+86400.,86400.)),TEXT("store fixture advances real clock to trading hours")))return;
   ShopCash=F.Budget();ShopFuses=F.InventoryCount(TEXT("fuse"));ShopRelays=F.InventoryCount(TEXT("relay"));ShopDelivered=F.Field->DeliveredParcels();ShopCargo=F.Field->CargoDeliveries();
   M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Go(8701,.4);return;
  }
  if(Stage==8701)
  {
   if(M->World->bBusy)return;
   Place(FVector(7983.8,0,2070.1));Go(87011,.3);return;
  }
  if(Stage==87011)
  {
   const FVector Counter=F.Field->InteractionPoint(TEXT("ff:shop"));
   if(!Check(FVector2D(M->Player->Pos.X-Counter.X,M->Player->Pos.Z-Counter.Z).Size()<2.4&&FMath::Abs(M->Player->FloorY-Counter.Y)<3.,TEXT("outside-wall rejection fixture is within ordinary counter distance and height range")))return;
   if(!Check(!F.Field->Purchase(TEXT("fuse"))&&F.Budget()==ShopCash&&F.Field->ParcelStage()==0,TEXT("ordinary purchase is rejected outside the shop wall even within counter range")))return;
   Place(FVector(7983.75,0,2067.2005));if(!AS47Props::IsDoorOpen(TEXT("gasDoor")))M->Inter->UseId(TEXT("town:gasDoor"));Go(87012,.6);return;
  }
  if(Stage==87012)
  {
   // Fixture placement through the production door, not an asserted walking traversal.
   Place(FVector(7981.8,0,2068.8));Go(87013,.3);return;
  }
  if(Stage==87013)
  {
   const FVector Counter=F.Field->InteractionPoint(TEXT("ff:shop"));Aim(Counter);
   if(!Check(FMath::Abs(M->Player->FloorY-5.446)<.3,TEXT("store fixture stands on the actual interior floor before opening counter")))return;
   Go(8702,.3);return;
  }
  if(Stage==8702)
  {
   Shot(TEXT("shop-interior-counter"));Go(8703,.3);return;
  }
  if(Stage==8703)
  {
   if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("ff:shop")&&bool(M->Player->OnUse),TEXT("native interaction ray reaches the supply counter inside the store")))return;
   M->Player->OnUse(TEXT("ff:shop"));
   if(!Check(M->UI->Modal(),TEXT("physical interior counter opens native shop menu")))return;
   ShopClick=0;Go(8704,.2);return;
  }
  if(Stage==8704)
  {
   const FString Label=ShopClick==1?TEXT("drive relay / $25"):TEXT("control fuse / $5");
   TArray<TSharedRef<SButton>> Buttons;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)PhotoButtons(Modal.ToSharedRef(),Label,Buttons);
   if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("enabled native purchase button: ")+Label))return;
#if !UE_BUILD_SHIPPING
   Buttons[0]->SimulateClick();
#else
   Finish(false,TEXT("native basket menu test requires development build"));return;
#endif
   ++ShopClick;const int32 Spent=ShopClick==1?5:ShopClick==2?30:35;
   if(!Check(M->UI->Modal()&&F.Budget()==ShopCash-Spent&&StockUnchanged()&&F.Field->ParcelStage()==1,TEXT("each purchase refreshes the open menu, charges exact price and keeps goods out of SARO stock")))return;
   if(ShopClick<3){Go(8704,.2);return;}
   if(!Check(Contents(),TEXT("one paid shop parcel contains two fuses and one relay")))return;
   Go(87041,.3);return;
  }
  if(Stage==87041)
  {
   Shot(TEXT("shop-three-items-menu"));Go(8705,.3);return;
  }
  if(Stage==8705)
  {
   M->UI->Close(true);const FVector Parcel=F.Field->InteractionPoint(TEXT("ff:parcel"));Place(FVector(7981.8,0,2068.8));Aim(Parcel);Go(87051,.3);return;
  }
  if(Stage==87051)
  {
   if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("ff:parcel"),TEXT("native interaction ray reaches the paid counter parcel")))return;
   M->Player->OnUse(TEXT("ff:parcel"));
   if(!Check(F.Field->ParcelStage()==2&&Contents()&&!F.Field->Purchase(TEXT("fuse"))&&F.Budget()==ShopCash-35,TEXT("pickup carries the complete paid basket and locks further remote purchases")))return;
   const FVector Cargo=F.Field->InteractionPoint(TEXT("ff:cargo"));Place(Cargo+FVector(1.4,0,0));F.Field->Use(TEXT("ff:cargo"));
   if(!Check(F.Field->ParcelStage()==3&&Contents()&&StockUnchanged(),TEXT("actual cargo callback loads the entire parcel without crediting stock")))return;
   if(!Restore(TEXT("shop-loaded-basket")))return;Go(8706,.4);return;
  }
  if(Stage==8706)
  {
   if(!Check(F.Field->ParcelStage()==3&&Contents()&&StockUnchanged()&&F.Budget()==ShopCash-35,TEXT("disk Continue retains paid quantities, loaded cargo and exact budget")))return;
   F.Director->Stop();M->World->Enter(TEXT("saro"));Go(8707,.4);return;
  }
  if(Stage==8707)
  {
   if(M->World->bBusy)return;M->World->PlaceTruck(TEXT("saro"));const FVector Cargo=F.Field->InteractionPoint(TEXT("ff:cargo"));Place(Cargo+FVector(1.4,0,0));F.Field->Use(TEXT("ff:cargo"));
   if(!Check(F.Field->ParcelStage()==2&&Contents()&&StockUnchanged(),TEXT("physical unloading keeps the same basket in hand before receiving")))return;
   const FVector Receiving=F.Field->InteractionPoint(TEXT("ff:delivery"));Place(Receiving+FVector(0,0,1.5));F.Field->Use(TEXT("ff:delivery"));
   if(!Check(F.Field->ParcelStage()==0&&F.Field->ParcelContents().IsEmpty()&&F.InventoryCount(TEXT("fuse"))==ShopFuses+2&&F.InventoryCount(TEXT("relay"))==ShopRelays+1&&F.Field->DeliveredParcels()==ShopDelivered+1&&F.Field->CargoDeliveries()==ShopCargo+1&&F.Budget()==ShopCash-35,TEXT("one garage delivery credits exact basket contents and one cargo transaction")))return;
   F.Field->Use(TEXT("ff:delivery"));M->UI->Close(true);
   if(!Check(F.InventoryCount(TEXT("fuse"))==ShopFuses+2&&F.InventoryCount(TEXT("relay"))==ShopRelays+1&&F.Field->DeliveredParcels()==ShopDelivered+1,TEXT("repeated receiving cannot duplicate basket stock or delivery credit")))return;
   if(!Restore(TEXT("shop-delivered-basket")))return;Go(8708,.3);return;
  }
  if(Stage==8708)
  {
   if(!Check(F.Field->ParcelStage()==0&&F.Field->ParcelContents().IsEmpty()&&F.InventoryCount(TEXT("fuse"))==ShopFuses+2&&F.InventoryCount(TEXT("relay"))==ShopRelays+1&&F.Budget()==ShopCash-35,TEXT("delivered disk Continue retains stock once with no paid parcel remaining")))return;
   const auto Legacy=S47J::Read(S47J::Write(F.Field->Save()));Legacy->RemoveField(TEXT("contents"));S47J::Set(Legacy,TEXT("item"),TEXT("fuse"));S47J::Set(Legacy,TEXT("payload"),1);F.Field->Begin(Legacy);
   if(!Check(F.Field->ParcelStage()==1&&F.Field->ParcelItem()==TEXT("fuse")&&F.Field->ParcelContents().Num()==1&&F.Field->ParcelContents().FindRef(TEXT("fuse"))==1&&F.InventoryCount(TEXT("fuse"))==ShopFuses+2&&F.Budget()==ShopCash-35,TEXT("explicit legacy singleton fixture becomes one paid fuse without charging or delivering it again")))return;
   // Isolated boundary fixture: replace the completed journey's stock/legacy parcel.
   Snapshot=S47J::Read(S47J::Write(F.Field->Save()));S47J::Set(Snapshot,TEXT("payload"),0);S47J::Set(Snapshot,TEXT("item"),TEXT(""));S47J::Set(Snapshot,TEXT("contents"),S47J::New());
   S47J::Set(S47J::Obj(Snapshot,TEXT("stock")),TEXT("fuse"),98);F.Field->Begin(Snapshot);
   F.Director->Stop();M->World->Enter(TEXT("diner"));Go(87081,.4);return;
  }
  if(Stage==87081)
  {
   if(M->World->bBusy)return;Place(FVector(7981.8,0,2068.8));Go(87082,.3);return;
  }
  if(Stage==87082)
  {
   ShopCash=F.Budget();ShopRelays=F.InventoryCount(TEXT("relay"));
   if(!Check(F.Field->Purchase(TEXT("fuse"))&&F.InventoryCount(TEXT("fuse"))==98&&F.Field->ParcelContents().FindRef(TEXT("fuse"))==1&&F.Budget()==ShopCash-5,TEXT("explicit stock-98 fixture buys one paid fuse up to combined cap99")))return;
   if(!Check(!F.Field->Purchase(TEXT("fuse"))&&F.Budget()==ShopCash-5&&F.Field->ParcelContents().FindRef(TEXT("fuse"))==1&&F.InventoryCount(TEXT("fuse"))==98,TEXT("purchase above combined cap99 is rejected without charge or content change")))return;
   if(!Check(F.Field->Purchase(TEXT("relay"))&&F.Budget()==ShopCash-30,TEXT("second supply remains purchasable when only fuse reaches cap")))return;
   F.Field->Use(TEXT("ff:parcel"));if(!Check(F.Field->ParcelStage()==2,TEXT("boundary fixture picks up its paid two-item parcel")))return;
   // Simulate an independent stock increase after purchase, retaining paid contents.
   Snapshot=S47J::Read(S47J::Write(F.Field->Save()));S47J::Set(S47J::Obj(Snapshot,TEXT("stock")),TEXT("fuse"),99);F.Field->Begin(Snapshot);
   ShopDelivered=F.Field->DeliveredParcels();ShopCargo=F.Field->CargoDeliveries();WorkBudget=F.Field->ServiceSupplyDeliveries();
   M->World->Enter(TEXT("saro"));Go(87083,.4);return;
  }
  if(Stage==87083)
  {
   if(M->World->bBusy)return;const FVector Receiving=F.Field->InteractionPoint(TEXT("ff:delivery"));Place(Receiving+FVector(0,0,1.5));F.Field->Use(TEXT("ff:delivery"));
   const auto& C=F.Field->ParcelContents();
   if(!Check(F.Field->ParcelStage()==2&&C.Num()==2&&C.FindRef(TEXT("fuse"))==1&&C.FindRef(TEXT("relay"))==1&&F.InventoryCount(TEXT("fuse"))==99&&F.InventoryCount(TEXT("relay"))==ShopRelays&&F.Budget()==ShopCash-30&&F.Field->DeliveredParcels()==ShopDelivered&&F.Field->CargoDeliveries()==ShopCargo&&F.Field->ServiceSupplyDeliveries()==WorkBudget,TEXT("one full stock item blocks the entire delivery without partial credit, parcel loss, charge or counters")))return;
   if(!Check(F.Field->Consume(TEXT("fuse")),TEXT("explicit boundary fixture frees one stock slot")))return;
   F.Field->Use(TEXT("ff:delivery"));
   if(!Check(F.Field->ParcelStage()==0&&F.Field->ParcelContents().IsEmpty()&&F.InventoryCount(TEXT("fuse"))==99&&F.InventoryCount(TEXT("relay"))==ShopRelays+1&&F.Budget()==ShopCash-30&&F.Field->DeliveredParcels()==ShopDelivered+1,TEXT("same blocked paid parcel delivers atomically once capacity is available")))return;
   Finish(true,TEXT("Store basket, Continue, legacy singleton, stock cap and atomic blocked delivery passed."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47CameraPickupTest")))
 {
  if(Age>90){Finish(false,TEXT("bounded physical camera test timeout"));return;}
  auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
  if(!C1||!M->FieldCamera||!M->Night||!M->Night->Album){Check(false,TEXT("production camera and album controllers available"));return;}
  if(Stage==0)
  {
   if(Age<3)return;
   M->UI->Close(true);F.Director->Stop();const auto Camera=M->Web->Obj(TEXT("room.fieldCamera"));
   if(!Check(!M->FieldCamera->bHave&&Camera&&Camera.WorldPosition().Equals(FVector(18.55,.89,-1.9),.001)&&!M->Inter->LabelOf(TEXT("fieldCamera")).IsEmpty(),TEXT("fresh Freeform has an unowned, labelled camera on the physical photolab archive bench")))return;
   C1->ToggleCamera();if(!Check(!M->FieldCamera->bRaised,TEXT("camera toggle cannot bypass physical collection")))return;
   Snapshot=S47J::Read(S47J::Write(M->Flow->GameState()));
   if(!Restore(TEXT("camera-unowned")))return;Go(8601,.3);return;
  }
  if(Stage==8601)
  {
   if(!Check(!M->FieldCamera->bHave&&!S47J::Bool(M->Flow->GameState(),TEXT("freeformCameraOwned")),TEXT("disk Continue preserves explicit unowned camera")))return;
   M->UI->Close(true);F.Director->Stop();const auto* It=M->Inter->Get(TEXT("fieldCamera"));
   if(!Check(It&&It->Object,TEXT("physical camera has the production registered interaction object")))return;
   const FVector Target=It->Object.WorldBox().GetCenter();M->Player->Place(17.25,-1.9,0);
   const FVector D=Target-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   Go(8602,.3);return;
  }
  if(Stage==8602)
  {
   const auto Camera=M->Web->Obj(TEXT("room.fieldCamera"));
   UE_LOG(LogTemp,Display,TEXT("S47 camera ray: picked=%s eye=%s target=%s box=%s visible=%d"),*M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE()),*S47Three::ToThree(M->Player->EyeUE()).ToString(),*Camera.WorldPosition().ToString(),*Camera.WorldBox().ToString(),Camera.Visible());
   Shot(TEXT("camera-before-pickup"));Go(86021,.3);return;
  }
  if(Stage==86021)
  {
   const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
   if(!Check(Picked==TEXT("fieldCamera")&&!M->Inter->LabelOf(Picked).IsEmpty()&&bool(M->Player->OnUse),TEXT("actual native ray selects the labelled photolab camera at pickup range")))return;
   Go(8603);return;
  }
  if(Stage==8603)
  {
   M->Player->OnUse(TEXT("fieldCamera"));
   if(!Check(M->FieldCamera->bHave&&M->Inter->LabelOf(TEXT("fieldCamera")).IsEmpty()&&M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())!=TEXT("fieldCamera"),TEXT("production use collects camera once and removes its pickup prompt/ray target")))return;
   Shot(TEXT("camera-after-pickup"));Go(8604,.3);return;
  }
  if(Stage==8604)
  {
   M->UI->Close(true);C1->ToggleCamera();
   if(!Check(M->FieldCamera->bRaised,TEXT("owned camera raises through production toggle route")))return;
   CameraShots=M->Night->Album->Shots().Num();CameraFilm=M->Night->Album->Left();C1->Shutter();
   if(!Check(M->Night->Album->Shots().Num()==CameraShots+1&&M->Night->Album->Left()==CameraFilm-1,TEXT("production shutter captures one native exposure and consumes one film frame")))return;
   const auto& Photo=M->Night->Album->Shots().Last();RetainedPhotoId=Photo.Id;RetainedPhotoUrl=Photo.Url;
   if(!Check(!Photo.bUnavailable&&!Photo.Url.IsEmpty()&&!Photo.IsDeveloped(),TEXT("new exposure retains native image and remains undeveloped")))return;
   M->FieldCamera->Raise(false);M->UI->Close(true);if(!Restore(TEXT("camera-owned")))return;Go(8605,.3);return;
  }
  if(Stage==8605)
  {
   const auto* Photo=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(M->FieldCamera->bHave&&S47J::Bool(M->Flow->GameState(),TEXT("freeformCameraOwned"))&&M->Inter->LabelOf(TEXT("fieldCamera")).IsEmpty()&&Photo&&Photo->Url==RetainedPhotoUrl&&M->Night->Album->Left()==CameraFilm-1,TEXT("owned disk Continue retains camera, exact exposure and film without duplicate pickup")))return;
   auto Legacy=S47J::Read(S47J::Write(Snapshot));Legacy->RemoveField(TEXT("freeformCameraOwned"));US47Flow::FStart Start;Start.bDev=true;Start.State=Legacy;M->Flow->StartGame(Start);Go(8606,.3);return;
  }
  if(Stage==8606)
  {
   if(!Check(M->FieldCamera->bHave&&S47J::Bool(M->Flow->GameState(),TEXT("freeformCameraOwned")),TEXT("explicit legacy Freeform fixture missing ownership field preserves formerly auto-granted camera")))return;
   US47Flow::FStart Story;Story.bDev=true;Story.Jump=TEXT("chapter1");M->Flow->StartGame(Story);Go(8690);return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PrinterLegacyQueueTest")))
 {
  if(Age>60){Finish(false,TEXT("bounded legacy printer queue timeout"));return;}
  auto Original=[&]()->const FS47Doc* {return M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==PrinterId+TEXT("-original");});};
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.Jobs().Num()>=2&&M->World->Area==TEXT("saro"),TEXT("legacy queue uses explicit isolated SARO fixture")))return;
   M->UI->Close(true);auto Save=S47J::Read(S47J::Write(F.Save()));auto Jobs=S47J::Arr(Save,TEXT("jobs"));
   for(int32 I=0;I<Jobs.Num();++I){auto O=Jobs[I]->AsObject();S47J::Set(O,TEXT("stage"),int32(J::New));S47J::Set(O,TEXT("control"),false);S47J::Set(O,TEXT("anomaly"),false);}
   auto Old=Jobs[0]->AsObject();PrinterId=S47J::Str(Old,TEXT("id"));
   S47J::Set(Old,TEXT("stage"),int32(J::Recorded));S47J::Set(Old,TEXT("phenomenon"),TEXT("duplicate_print"));S47J::Set(Old,TEXT("printed"),true);S47J::Set(Old,TEXT("anomaly"),true);S47J::Set(Old,TEXT("control"),true);S47J::Set(Old,TEXT("controlAt"),8.);S47J::Set(Old,TEXT("elapsed"),18.);
   // Actual pre-3b6a6c1c schema: the original was a Story document, with no
   // capture-counter line or per-job original/duplicate snapshots or feed fields.
   for(const TCHAR* Key:{TEXT("originalPage"),TEXT("originalStamp"),TEXT("duplicatePage"),TEXT("duplicateStamp"),TEXT("duplicateCollected"),TEXT("duplicateClock"),TEXT("duplicateInterval"),TEXT("chartFeed"),TEXT("packetFeed"),TEXT("duplicateFeed"),TEXT("referenceClock"),TEXT("receiverClock")})Old->RemoveField(Key);
   const int32 Source=F.Jobs()[0].Source;PrinterStamp=TEXT("SARO / ")+PrinterId;
   PrinterOriginal=PrinterStamp+TEXT("\n")+FS47Observing::Runs()[Source].Source+FString::Printf(TEXT("\nACTUAL ARRAY / QUALITY %.0f %%\nF %.3f MHz / AZ %.1f / EL %.1f\nINTEGRATION %.1f SECONDS\n"),F.Jobs()[0].Quality*100,FS47Observing::Runs()[Source].P.F,(FS47Observing::Runs()[Source].P.Az[0]+FS47Observing::Runs()[Source].P.Az[1])/2,FS47Observing::Runs()[Source].El,18.)+TEXT("PROVENANCE: ORIGINAL RX RECORD. A separate duplicate was received with an inconsistent timestamp; this original is retained unchanged.\n");
   FS47Doc D;D.Id=PrinterId+TEXT("-original");D.Title=TEXT("Original observation: ")+PrinterId;D.Kind=TEXT("printout");D.Stamp=PrinterStamp;D.Page=PrinterOriginal;D.Transcript=D.Page;M->Story->AddDoc(D);
   auto Later=Jobs[1]->AsObject();S47J::Set(Later,TEXT("stage"),int32(J::Chart));S47J::Set(Later,TEXT("chartFeed"),0.);S47J::Set(Later,TEXT("printed"),false);S47J::Set(Later,TEXT("elapsed"),18.);
   S47J::Set(Save,TEXT("selected"),0);S47J::Set(Save,TEXT("powerFault"),false);F.Begin(Save);F.Director->Stop();PrinterBudget=F.Budget();
   const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));M->Player->Place(P.X,P.Z+1.1,0);
   if(!Restore(TEXT("legacy-duplicate-with-later-chart")))return;Go(8550,.3);return;
  }
  if(Stage==8550)
  {
   const auto* D=Original();if(!Check(D&&D->Page==PrinterOriginal&&D->Stamp==PrinterStamp&&!D->Page.Contains(TEXT("CAPTURE CLOCK")),TEXT("real legacy original format survives disk Continue unchanged")))return;
   F.Use(TEXT("printer"));M->UI->Close(true);
   if(!Check(F.SelectJob(1),TEXT("later observation can be selected after legacy duplicate collection attempt")))return;
   Go(8551,6.5);return;
  }
  if(Stage==8551)
  {
   F.Use(TEXT("printer"));
   const auto* D=Original();if(!Check(D&&D->Page==PrinterOriginal&&D->Stamp==PrinterStamp&&F.Budget()==PrinterBudget,TEXT("legacy collection attempt neither rewrites original nor pays again")))return;
   if(!Check(M->Story->Docs().ContainsByPredicate([&](const FS47Doc& X){return X.Id==F.Jobs()[1].Id+TEXT("-original");})&&F.Jobs()[1].Stage==J::Recorded,TEXT("legacy duplicate without counters must not block the later valid printer sheet")))return;
   M->UI->Close(true);if(!Restore(TEXT("legacy-queue-after-later-collection")))return;Go(8552,.3);return;
  }
  if(Stage==8552)
  {
   const auto* D=Original();if(!Check(D&&D->Page==PrinterOriginal&&D->Stamp==PrinterStamp&&F.Jobs()[1].Stage==J::Recorded&&F.Budget()==PrinterBudget,TEXT("later collection and unchanged legacy original survive another Continue")))return;
   Finish(true,TEXT("Legacy duplicate queue regression passed."));return;
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PrinterTest")))
 {
  if(Age>100){Finish(false,TEXT("bounded printer fixture timeout"));return;}
  auto Doc=[&](const FString& Suffix){return M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==PrinterId+Suffix;});};
  auto Power=[&](bool On){auto Save=S47J::Read(S47J::Write(F.Save()));S47J::Set(Save,TEXT("powerFault"),!On);F.Begin(Save);F.Director->Stop();M->UI->Close(true);};
  auto Paper=[&](){const auto O=M->Web->Obj(TEXT("room.printerPaper"));const auto* N=O?M->Web->Node(O.I):nullptr;return N&&N->bVis&&FMath::Abs(N->S.Y-FMath::Max(.001,F.PrinterProgress()))<.005;};
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&F.Jobs().Num()>=2&&M->World->Area==TEXT("saro"),TEXT("printer test uses isolated SARO prepared-job fixture")))return;
   M->UI->Close(true);auto Save=S47J::Read(S47J::Write(F.Save()));auto Jobs=S47J::Arr(Save,TEXT("jobs"));
   for(int32 I=0;I<Jobs.Num();++I){auto O=Jobs[I]->AsObject();S47J::Set(O,TEXT("stage"),int32(I<2?J::Chart:J::New));S47J::Set(O,TEXT("chartFeed"),0.);S47J::Set(O,TEXT("anomaly"),false);S47J::Set(O,TEXT("paid"),false);S47J::Set(O,TEXT("elapsed"),18.);S47J::Set(O,TEXT("referenceClock"),1234.5);}
   S47J::Set(Save,TEXT("selected"),0);S47J::Set(Save,TEXT("powerFault"),false);F.Begin(Save);F.Director->Stop();PrinterId=F.Jobs()[0].Id;PrinterBudget=F.Budget();
   const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));M->Player->Place(P.X,P.Z+1.1,0);const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   if(!Check(!F.CollectChart()&&!Doc(TEXT("-original")),TEXT("unfinished paper cannot create an original document")))return;
   Go(8501);return;
  }
  if(Stage==8501)
  {
   if(F.PrinterProgress()<.25)return;
   if(!Check(F.PrinterProgress()<1&&F.PrinterFeedbackActive()&&Paper()&&!Doc(TEXT("-original")),TEXT("native paper advances with active printer audio loop and no premature original")))return;
   const auto Loop=M->Audio->InspectLoop(TEXT("ffPrinter"));
   if(!Check(Loop.IsSet()&&Loop->bPlaying&&Loop->bLoop&&Loop->Gain>0&&Loop->Buffer.IsValid(),TEXT("printer recipe has an actual playing loop with gain and a decoded audio buffer")))return;
   Shot(TEXT("printer-partial"));PrinterHeld=F.PrinterProgress();
   if(!Check(F.SelectJob(1)&&F.PrinterJobId()==PrinterId,TEXT("changing selected job preserves the started printer sheet")))return;
   Power(false);if(!Restore(TEXT("printer-mid-feed-power-off")))return;Go(8502,.8);return;
  }
  if(Stage==8502)
  {
   if(!Check(!F.HasPowerForReactiveLight()&&!F.PrinterFeedbackActive()&&FMath::Abs(F.PrinterProgress()-PrinterHeld)<.001&&Paper()&&!F.CollectChart()&&!Doc(TEXT("-original")),TEXT("disk Continue preserves partial paper; outage stops feed and audio without creating document")))return;
   const auto Loop=M->Audio->InspectLoop(TEXT("ffPrinter"));
   if(!Check(!Loop.IsSet()||!Loop->bPlaying||Loop->Gain<=0,TEXT("power interruption leaves no playing recorded printer loop")))return;
   const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));M->Player->Place(P.X,P.Z+1.1,0);const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   Power(true);Go(8503);return;
  }
  if(Stage==8503)
  {
   if(F.PrinterProgress()<1)return;
   if(!Check(!F.PrinterFeedbackActive()&&Paper()&&!Doc(TEXT("-original")),TEXT("completed native sheet stops printer loop and waits for collection")))return;
   Shot(TEXT("printer-ready"));Go(85031,.3);return;
  }
  if(Stage==85031)
  {
   Power(false);
   if(!Check(F.CollectChart()&&Doc(TEXT("-original"))&&F.CurrentJob()->Id==PrinterId&&F.Budget()==PrinterBudget,TEXT("ready original can be collected without power; queue identity and budget are retained")))return;
   PrinterOriginal=F.CurrentJob()->OriginalPage;PrinterStamp=F.CurrentJob()->OriginalStamp;M->UI->Close(true);
   if(!Check(!PrinterOriginal.IsEmpty()&&F.PrinterJobId()==F.Jobs()[1].Id&&!F.CollectChart(),TEXT("collection reveals next unpaid sheet without duplicating first original")))return;
   auto Save=S47J::Read(S47J::Write(F.Save()));auto Jobs=S47J::Arr(Save,TEXT("jobs"));S47J::Set(Jobs[1]->AsObject(),TEXT("stage"),int32(J::New));auto O=Jobs[0]->AsObject();S47J::Set(O,TEXT("stage"),int32(J::Registered));S47J::Set(O,TEXT("anomaly"),true);S47J::Set(O,TEXT("packetFeed"),0.);S47J::Set(Save,TEXT("powerFault"),false);F.Begin(Save);F.Director->Stop();
   if(!Check(!F.CollectChart()&&!Doc(TEXT("-packet")),TEXT("registered packet must finish its own physical feed")))return;
   Go(8504);return;
  }
  if(Stage==8504)
  {
   if(F.PrinterProgress()<1)return;
   Power(false);
   if(!Check(F.CollectChart()&&Doc(TEXT("-packet"))&&F.CurrentJob()->OriginalPage==PrinterOriginal&&F.CurrentJob()->OriginalStamp==PrinterStamp&&F.Budget()==PrinterBudget,TEXT("packet collection without power preserves original provenance and grants no extra reward")))return;
   M->UI->Close(true);auto Save=S47J::Read(S47J::Write(F.Save()));auto O=S47J::Arr(Save,TEXT("jobs"))[0]->AsObject();S47J::Set(O,TEXT("stage"),int32(J::Recorded));S47J::Set(O,TEXT("phenomenon"),TEXT("duplicate_print"));S47J::Set(O,TEXT("control"),true);S47J::Set(O,TEXT("controlAt"),8.);S47J::Set(O,TEXT("duplicateFeed"),0.);S47J::Set(Save,TEXT("powerFault"),false);F.Begin(Save);F.Director->Stop();
   if(!Check(!F.CollectChart()&&!Doc(TEXT("-duplicate")),TEXT("separate duplicate cannot be collected before its own feed")))return;
   Go(8505);return;
  }
  if(Stage==8505)
  {
   if(F.PrinterProgress()<1)return;
   Power(false);
   if(!Check(F.CollectChart()&&Doc(TEXT("-duplicate"))&&F.CurrentJob()->bDuplicateCollected&&F.CurrentJob()->OriginalPage==PrinterOriginal&&F.CurrentJob()->OriginalStamp==PrinterStamp&&F.CurrentJob()->DuplicatePage!=PrinterOriginal&&F.Budget()==PrinterBudget&&!F.CollectChart(),TEXT("finished duplicate is separately retained without power, original mutation or repeat reward")))return;
   M->UI->Close(true);if(!Restore(TEXT("printer-all-collected")))return;Go(8506,.3);return;
  }
  if(Stage==8506)
  {
   if(!Check(F.PrinterProgress()<0&&!F.PrinterFeedbackActive()&&F.Jobs()[0].OriginalPage==PrinterOriginal&&F.Jobs()[0].OriginalStamp==PrinterStamp&&Doc(TEXT("-original"))&&Doc(TEXT("-packet"))&&Doc(TEXT("-duplicate"))&&F.Budget()==PrinterBudget,TEXT("disk Continue retains three distinct papers and original provenance with empty stopped printer")))return;
   Finish(true,TEXT("Native printer fixture passed: feed/audio state, outage, Continue, queue, original/packet/duplicate identity; no listening acceptance."));return;
  }
  return;
 }
 // One real trip transition, not a second test framework or a simulated bBusy flag.
 if(FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTripHoldTest")))
 {
  if(Age>120){Finish(false,TEXT("trip-hold subset timeout"));return;}
  auto Angles=[&](){TArray<double> V;for(int32 I=0;I<M->Web->DishCount();++I)V.Append({M->Web->DishAz(I),M->Web->DishEl(I)});return V;};
  if(Stage==0)
  {
   if(Age<3)return;
   if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->IsFreeform()&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&!M->World->bBusy&&F.Director&&F.Field&&F.Field->ParcelStage()==0,TEXT("isolated fresh SARO Freeform case, no cargo or competing trip")))return;
   M->UI->Close(true);F.Director->Stop();
   if(!Check(F.SelectJob(1)&&F.CurrentJob()&&F.CurrentJob()->Source==1,TEXT("existing generated pulsar work order selected")))return;
   F.SetAnomaly(true);const auto P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("actual selected work card supplies explicit tuning fixture")))return;
   auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
   if(!Check(F.PointSelected(),TEXT("production pointing command accepted; no encoder snap or job-stage injection")))return;Go(9001);return;
  }
  if(Stage==9001)
  {
   if(!F.StartAcquisition())return;
   if(!Check(F.CurrentJob()->Stage==J::Integrating,TEXT("settled actual array starts the real timed recording")))return;Go(9002);return;
  }
  if(Stage==9002)
  {
   if(F.CurrentJob()->Stage!=J::Chart||F.PrinterProgress()<1)return;
   if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Recorded&&!F.CurrentJob()->OriginalPage.IsEmpty(),TEXT("actual completed recording and printer original retained")))return;
   M->UI->Close(true);if(!Check(F.StartControl()&&F.CurrentJob()->bControl&&F.CurrentJob()->ControlAt==0,TEXT("real eight-second internal reference starts before the trip")))return;
   Snapshot=S47J::New();S47J::Set(Snapshot,TEXT("clock"),F.Clock());S47J::Set(Snapshot,TEXT("control"),F.CurrentJob()->ControlAt);S47J::Set(Snapshot,TEXT("job"),F.CurrentJob()->Id);S47J::Set(Snapshot,TEXT("original"),F.CurrentJob()->OriginalPage);S47J::Set(Snapshot,TEXT("angles"),Angles());
   // Explicit diagnostic command during an INTERNAL reference; not a physical pointing lesson.
   const double Az=M->Web->DishAz(2)+6,El=M->Web->DishEl(2);M->Web->PointDishes(Az,El,2.2);
   S47J::Set(Snapshot,TEXT("fixture_target"),TArray<double>{Az,El});
   M->World->Roam(TEXT("saro"));
   if(!Check(M->World->bBusy&&!M->World->bDriving&&F.CurrentJob()->bControl,TEXT("ordinary Roam entered actual StartTrip busy transition without stopping the job")))return;
   Go(9003);return;
  }
  if(Stage==9003)
  {
   const auto* Work=F.CurrentJob();const auto ActualAngles=Angles(),BeforeAngles=S47J::Nums(Snapshot,TEXT("angles"));
   bool SameAngles=ActualAngles.Num()==BeforeAngles.Num()&&ActualAngles.Num()>=6;
   for(int32 I=0;I<ActualAngles.Num()&&I<BeforeAngles.Num();++I)SameAngles&=FMath::Abs(ActualAngles[I]-BeforeAngles[I])<1e-8;
   auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("actual StartTrip frame"));S47J::Set(Row,TEXT("busy"),M->World->bBusy);S47J::Set(Row,TEXT("driving"),M->World->bDriving);S47J::Set(Row,TEXT("clock"),F.Clock());S47J::Set(Row,TEXT("control"),Work?Work->ControlAt:-1);S47J::Set(Row,TEXT("native_angles"),ActualAngles);Evidence.Add(MakeShared<FJsonValueObject>(Row));
   if(M->World->bBusy)
   {
    ++TripHoldSamples;
    if(!Check(Work&&Work->Id==S47J::Str(Snapshot,TEXT("job"))&&Work->Stage==J::Recorded&&Work->bControl&&Work->OriginalPage==S47J::Str(Snapshot,TEXT("original"))&&FMath::Abs(Work->ControlAt-S47J::Num(Snapshot,TEXT("control")))<1e-9&&FMath::Abs(F.Clock()-S47J::Num(Snapshot,TEXT("clock")))<1e-9&&SameAngles,TEXT("actual busy frame preserves original, control progress, clock and every native encoder")))return;
    return;
   }
   if(!Check(TripHoldSamples>0&&M->World->bDriving&&Work&&Work->Id==S47J::Str(Snapshot,TEXT("job"))&&Work->bControl,TEXT("real scheduled StartTrip callback ended busy, entered driving and retained the same active job")))return;
   Go(9004,.6);return;
  }
  if(Stage==9004)
  {
   const auto* Work=F.CurrentJob();const auto A=Angles(),B=S47J::Nums(Snapshot,TEXT("angles"));double Moved=0;for(int32 I=0;I<A.Num()&&I<B.Num();++I)Moved=FMath::Max(Moved,FMath::Abs(A[I]-B[I]));
   if(!Check(Work&&Work->Id==S47J::Str(Snapshot,TEXT("job"))&&Work->ControlAt>S47J::Num(Snapshot,TEXT("control"))+.1&&Work->OriginalPage==S47J::Str(Snapshot,TEXT("original"))&&F.Clock()>S47J::Num(Snapshot,TEXT("clock"))&&Moved>.05&&!F.Held()&&!F.ArrayMotionHeld(),TEXT("same control, world clock and actual native motors resume after the real transition")))return;
   auto Row=S47J::New();S47J::Set(Row,TEXT("kind"),TEXT("resumed"));S47J::Set(Row,TEXT("clock"),F.Clock());S47J::Set(Row,TEXT("control"),Work->ControlAt);S47J::Set(Row,TEXT("native_angles"),A);Evidence.Add(MakeShared<FJsonValueObject>(Row));
   if(!Check(M->World->GetOutHere(),TEXT("ordinary stationary exit ends the test trip")))return;Go(9005,.6);return;
  }
  if(Stage==9005)
  {
   if(M->World->bBusy||M->Story->IsCinematic()||F.CurrentJob()->ControlAt<8)return;
   if(!Check(!M->World->bDriving&&F.CurrentJob()->Id==S47J::Str(Snapshot,TEXT("job"))&&F.CurrentJob()->ControlAt==8&&F.CurrentJob()->OriginalPage==S47J::Str(Snapshot,TEXT("original")),TEXT("real internal reference completes after resumption; retained original and ordinary on-foot state remain")))return;
   Finish(true);return;
  }
  return;
 }
 // Explicit short photographic-economy fixture. This is not a natural encounter or OS-input beta.
 if(FParse::Param(FCommandLine::Get(),TEXT("S47PhotoEconomyTest")))
 {
  if(Stage==0)
  {
   if(Age<3)return;
   if(!M->FieldCamera->bHave)
   {
    M->UI->Close(true);M->Player->Place(17.25,-1.9,0);M->Inter->UseId(TEXT("fieldCamera"));
    if(!Check(M->FieldCamera->bHave,TEXT("photo-economy fixture explicitly collects its camera prerequisite at the lab")))return;
   }
   PhotoEvent=TEXT("landing_trace");FParse::Value(FCommandLine::Get(),TEXT("S47PhotoEconomyEvent="),PhotoEvent);
   if(!Check(PhotoEvent==TEXT("landing_trace")||PhotoEvent==TEXT("feeder_presence"),TEXT("photographic fixture accepts only landing_trace or opt-in feeder_presence")))return;
   if(PhotoEvent==TEXT("feeder_presence"))
   {
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Night&&M->Night->Flashlight&&M->Night->Glimpses&&M->FieldCamera&&F.Field,TEXT("night figure fixture has isolated native camera, flashlight and body systems")))return;
    const double Hour=FMath::Fmod(F.Clock(),86400.);if(!Check(F.TestAdvance(FMath::Fmod(22*3600.-Hour+86400.,86400.)),TEXT("explicit fixture advances actual world and Story clock to night")))return;
    M->UI->Close(true);M->Night->Flashlight->bHave=true;if(M->Night->Flashlight->bOn)M->Night->Flashlight->Toggle();
    M->World->Enter(TEXT("road"));Go(8090,1);return;
   }
   if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&F.Director->Force(TEXT("landing_trace")),TEXT("isolated photographic fixture starts a real native field trace")))return;
   PhotoBudget=F.Budget();const FVector P=F.Director->EventAnchor();M->Player->Place(P.X-1.5,P.Z,0);
   const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(8001,1);return;
  }
  if(Stage==8090)
  {
   if(!Check(M->Web->Area()==TEXT("road"),TEXT("night feeder fixture loads the actual road site")))return;
   const FVector P=F.Field->InteractionPoint(TEXT("ff:feeder"));M->Player->Place(P.X,P.Z+4.,0);
   if(!Check(F.Director->Force(PhotoEvent),TEXT("explicit Force starts feeder presence; natural scheduling is not tested")))return;
   PhotoBudget=F.Budget();Go(8091,.2);return;
  }
  if(Stage==8091)
  {
   if(!PosePhotoFigure()){if(Age-At>40)Check(false,TEXT("no rendered ControlledPhotoBody within bounded night fixture wait"));return;}
   Go(8092,.35);return;
  }
  if(Stage==8092)
  {
   if(!Check(!M->Night->Flashlight->bOn&&M->Night->Flashlight->Intensity()<.01,TEXT("first night exposure uses actual flashlight OFF")))return;
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Dark=M->Night->Album->Expose();LogPhotoFrame(TEXT("lamp-off"),Dark);M->FieldCamera->Raise(false);
   if(!Check(!Dark.bUnavailable&&!Dark.Url.IsEmpty(),TEXT("lamp-off frame is a real retained native exposure, independent of qualification")))return;
   Shot(TEXT("photo-night-lamp-off"));Go(80921,.2);return;
  }
  if(Stage==80921)
  {
   if(!Check(M->Night->Flashlight->Toggle(),TEXT("production flashlight toggle enables lamp for second exposure")))return;
   Go(8093,.6);return;
  }
  if(Stage==8093)
  {
   if(!Check(PosePhotoFigure(),TEXT("lamp-on exposure still has a rendered controlled body")))return;
   Go(8094,.25);return;
  }
  if(Stage==8094)
  {
   const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
   if(!Check((Hour>=19||Hour<6)&&M->Night->Flashlight->bOn&&M->Night->Flashlight->Intensity()>0,TEXT("qualifying figure exposure occurs at actual night with native lamp ON")))return;
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Lit=M->Night->Album->Expose();LogPhotoFrame(TEXT("lamp-on"),Lit);M->FieldCamera->Raise(false);
   if(!Check(!Lit.bUnavailable&&!Lit.Url.IsEmpty()&&F.Director->LastPhotoSubjectMean()>=15&&F.Director->PhotoClaims().Num()==1&&Lit.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& Target){return Target.Key==TEXT("freeform:")+F.Director->PhotoClaims()[0].Id;})&&F.Budget()==PhotoBudget,TEXT("lamp-on exposure has actual incident tag and qualifying subject light; incident retains one claim without shutter payment")))return;
   const auto& Claim=F.Director->PhotoClaims()[0];const auto* Original=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==Claim.PhotoId;});
   if(!Check(Original&&!Original->bUnavailable&&!Original->Url.IsEmpty(),TEXT("qualifying night claim references an actual retained exposure")))return;
   PhotoClaimId=Claim.Id;RetainedPhotoId=Original->Id;RetainedPhotoUrl=Original->Url;Shot(TEXT("photo-night-lamp-on"));Go(8002,.25);return;
  }
  if(Stage==8001)
  {
   if(!Check(F.Director->PhysicalObjectPresented(),TEXT("photographic subject has a real visible native component")))return;
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);
   if(!Check(!Photo.bUnavailable&&!Photo.Url.IsEmpty()&&F.Director->PhotoClaims().Num()==1&&F.Budget()==PhotoBudget,TEXT("rendered photograph queues one documentation claim without shutter payment")))return;
   PhotoClaimId=F.Director->PhotoClaims()[0].Id;RetainedPhotoId=Photo.Id;RetainedPhotoUrl=Photo.Url;
   Shot(TEXT("photo-evidence"));Go(8002,.5);return;
  }
  if(Stage==8002)
  {
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();M->Night->Album->Expose();M->FieldCamera->Raise(false);
   if(!Check(F.Director->PhotoClaims().Num()==1&&F.Budget()==PhotoBudget&&!F.Director->RegisterPhotoClaim(PhotoClaimId),TEXT("duplicate exposure does not mint another claim and field registration is refused")))return;
   M->Story->Note(TEXT("Legacy case timeline retained: original 02:13 references and photographs are unchanged. Any unstored printout acquisition time is marked unavailable."));
   M->Story->Note(TEXT("Retained field-test note."));
   if(!Restore(TEXT("photo-unpaid")))return;Go(8003,1);return;
  }
  if(Stage==8003)
  {
   if(!Check(F.Director->PhotoClaims().Num()==1&&!F.Director->PhotoClaims()[0].bPaid,TEXT("unpaid photo claim survives full disk and Flow restore")))return;
   if(!Check(M->Story->Notes().Contains(TEXT("Retained field-test note."))&&!M->Story->Notes().ContainsByPredicate([](const FString& N){return N.StartsWith(TEXT("Legacy case timeline retained:"));}),TEXT("Freeform Continue removes only the erroneous Story migration notice and preserves field notes")))return;
   if(PhotoEvent==TEXT("feeder_presence")){M->World->Enter(TEXT("saro"));Go(80031,1);return;}
   const FVector OpsAt=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(OpsAt.X,OpsAt.Z+1,0);Go(8004,.4);return;
  }
  if(Stage==80031)
  {
   if(!Check(M->Web->Area()==TEXT("saro"),TEXT("night photo fixture returns to actual SARO OPS after disk restore")))return;
   const FVector OpsAt=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(OpsAt.X,OpsAt.Z+1,0);Go(8004,.4);return;
  }
  if(Stage==8004)
  {
   auto& AlbumShots=M->Night->Album->Data.Shots;const auto Retained=AlbumShots;
   AlbumShots.Reset();const bool MissingRefused=!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget;AlbumShots=Retained;
   if(!Check(MissingRefused,TEXT("OPS refuses a claim whose retained original is missing without consuming its payment")))return;
   if(!Check(F.CanRegisterPhotoEvidence()&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget,TEXT("physical powered OPS refuses the undeveloped original without consuming payment")))return;
   const auto* Original=M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(Original&&Original->LabStage==0,TEXT("exposed photograph remains undeveloped through full disk Continue")))return;
   Snapshot=S47J::New();S47J::Set(Snapshot,TEXT("photoClock"),Original->Clock);S47J::Set(Snapshot,TEXT("photoTargets"),Original->Targets.Num());
   M->UI->Close(true);M->Web->SetDoorOpen(TEXT("lab"),false,true);
   const auto* Bench=M->Inter->Get(TEXT("wetBench"));if(!Check(Bench&&bool(Bench->Object),TEXT("photo lab uses the existing physical wet-bench object")))return;
   const FVector BenchAt=Bench->Object.WorldPosition();M->Player->Place(BenchAt.X,BenchAt.Z+1,0);Go(80040,.4);return;
  }
  if(Stage==80040)
  {
   const auto* Original=M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(Original&&Original->Url==RetainedPhotoUrl&&Original->Clock==S47J::Num(Snapshot,TEXT("photoClock"))&&Original->Targets.Num()==S47J::Int(Snapshot,TEXT("photoTargets")),TEXT("physical processing retains original identity, image, timestamp and incident targets")))return;
   if(Original->IsDeveloped())
   {
    if(!Restore(TEXT("photo-developed")))return;Go(80044,.5);return;
   }
   if(!Check(!M->Inter->LabelOf(TEXT("wetBench")).IsEmpty(),TEXT("physical Freeform wet bench exposes an actionable label")))return;
   M->Inter->UseId(TEXT("wetBench"));Go(80041,.2);return;
  }
  if(Stage==80041)
  {
   const auto* Pending=M->Night->Album->PendingLabPhoto();if(!Check(Pending!=nullptr,TEXT("wet bench retains a pending exposed original")))return;
   const FString PendingId=Pending->Id;const int32 Before=Pending->LabStage;
   const FString Label=Before==0?TEXT("LOAD SEALED FILM INTO TANK"):Before==1?TEXT("TRANSFER CONTACT PRINT TO FIXER"):TEXT("COLLECT AND EXAMINE PRINT");
   TArray<TSharedRef<SButton>> Buttons;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)PhotoButtons(Modal.ToSharedRef(),Label,Buttons);
   if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("unique enabled native wet-bench action: ")+Label))return;
#if !UE_BUILD_SHIPPING
   Buttons[0]->SimulateClick();
#else
   Finish(false,TEXT("physical photo test requires development build"));return;
#endif
   const auto* Processed=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==PendingId;});
   if(!Check(Processed&&Processed->LabStage==Before+1&&F.Budget()==PhotoBudget,TEXT("native wet-bench action advances one stage without crediting OPS payment")))return;
   if(PendingId==RetainedPhotoId&&Before==0)
   {
    if(!Restore(TEXT("photo-in-tank")))return;Go(80042,.5);return;
   }
   Go(80040,.2);return;
  }
  if(Stage==80042)
  {
   const auto* Original=M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(Original&&Original->LabStage==1&&!Original->IsDeveloped(),TEXT("tank-processing state survives full disk Continue without becoming a finished print")))return;
   Go(80040,.2);return;
  }
  if(Stage==80044)
  {
   const auto* Original=M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(Original&&Original->LabStage==3&&Original->Url==RetainedPhotoUrl,TEXT("collected original remains developed after full disk Continue")))return;
   const FVector OpsAt=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(OpsAt.X,OpsAt.Z+1,0);Go(80045,.4);return;
  }
  if(Stage==80045)
  {
   if(!Check(F.CanRegisterPhotoEvidence()&&F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("physical powered OPS registers developed original and credits exactly $20")))return;
   if(!Check(!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("repeat registration cannot pay the same incident twice")))return;
   if(!Restore(TEXT("photo-paid")))return;Go(8005,1);return;
  }
  if(Stage==8005)
  {
   const auto* Original=M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});
   if(!Check(F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].bPaid&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20&&Original&&Original->Url==RetainedPhotoUrl,TEXT("paid claim, original image and exactly-once balance survive Continue")))return;
   Finish(true,TEXT("photographic economy fixture: actual rendered native subject, duplicate guard, physical wet-bench tank/fixer/collection, OPS and exposed/processing/developed/paid disk Continue"));return;
  }
  return;
 }
 if(Stage==0)
 {
  if(Age<3)return;
  FString From;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47FreeformTestFrom="),From))
  {
   if(!Check(M->Flow->bDevRun&&From.StartsWith(TEXT("/tmp/S47-freeform-")),TEXT("field subset requires an isolated developer fixture")))return;
   FString Text;if(!Check(FFileHelper::LoadFileToString(Text,*From),TEXT("previous third-day fixture read for field-only iteration")))return;
   const auto State=S47J::Read(Text);if(!Check(State&&S47J::Str(State,TEXT("sessionMode"))==TEXT("freeform"),TEXT("field fixture retains Freeform identity")))return;
   bFieldResume=true;US47Flow::FStart Start;Start.bDev=true;Start.State=State;M->Flow->StartGame(Start);Go(20,.4);return;
  }
  if(!Check(M->IsFreeform()&&!M->Horror->Enabled()&&!M->Observing->Enabled()&&!M->Spine->Enabled()&&M->Story->ChapterKey==TEXT("freeform"),TEXT("Freeform starts without authored Story controllers")))return;
  if(!Check(F.Day()==1&&F.Budget()==120&&F.Jobs().Num()==3&&F.Field&&F.Director,TEXT("day one has three jobs, starting budget and physical/director adapters")))return;
  F.Director->Stop();F.OpenOperations();Shot(TEXT("operations"));Go(1,.4);return;
 }
 if(Stage==1)
 {
  M->UI->Close(true);F.SelectJob(JobIndex);const auto P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("selected job supplies real receiver profile")))return;
  auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
  if(JobIndex==1)F.SetAnomaly(true);InitialAz=M->Web->DishAz(2);InitialEl=M->Web->DishEl(2);
  if(!Check(!F.StartAcquisition()&&F.PointSelected(),TEXT("receiver cannot acquire before a physical pointing command")))return;
  F.OpenTracking();Go(2,.5);return;
 }
 if(Stage==2)
 {
  if(!Check(FMath::Abs(M->Web->DishAz(2)-InitialAz)>.01||FMath::Abs(M->Web->DishEl(2)-InitialEl)>.01,TEXT("actual native antenna encoders move while tracking telemetry is open")))return;
  Shot(TEXT("tracking"));M->UI->Close(true);Go(3,.1);return;
 }
 if(Stage==3)
 {
  if(!F.StartAcquisition())return;
  if(!Check(F.CurrentJob()->Stage==J::Integrating,TEXT("settled actual array permits the timed observation")))return;
  if(!Restore(TEXT("mid-acquisition")))return;F.Director->Stop();Go(4,.4);return;
 }
 if(Stage==4)
 {
  if(F.CurrentJob()->Stage!=J::Chart||F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==J::Recorded,TEXT("completed measurement retains an original chart")))return;
  M->UI->Close(true);
  if(JobIndex==1){if(!Check(F.StartControl(),TEXT("physical receiver control starts a separate timed reference")))return;Go(5);return;}
  Go(9);return;
 }
 if(Stage==5)
 {
  if(F.CurrentJob()->ControlAt<8)return;
  if(F.AudioDesk) { const FVector P=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original")); M->Player->Place(P.X,P.Z-1.2,0); }
  F.OpenAudio();if(!Check(!F.AudioAction(TEXT("confirm")),TEXT("comparison cannot be granted before actual playback")))return;
  F.AudioAction(TEXT("original"));F.AudioAction(TEXT("play"));Go(6);return;
 }
 if(Stage==6)
 {
  if(F.AudioPlaying())return;
  if(!Check(F.CurrentJob()->OriginalCovered>=18,TEXT("original recording really played through")))return;
  F.AudioAction(TEXT("control"));F.AudioAction(TEXT("play"));Go(7);return;
 }
 if(Stage==7)
 {
  if(F.AudioPlaying())return;
  if(!Check(F.CurrentJob()->ControlCovered>=8,TEXT("independent control recording really played through")))return;
  F.AudioAction(TEXT("original"));F.SeekAudio(9.1);F.AudioAction(TEXT("mark_start"));F.SeekAudio(13.0);F.AudioAction(TEXT("mark_end"));Shot(TEXT("audio-comparison"));Go(8,.3);return;
 }
 if(Stage==8)
 {
  if(!Check(F.AudioAction(TEXT("confirm"))&&F.CurrentJob()->bAudioReviewed,TEXT("supported interval is retained only after both recordings")))return;
  M->UI->Close(true);if(!Restore(TEXT("reviewed-recordings")))return;F.Director->Stop();Go(9);return;
 }
 if(Stage==9)
 {
  WorkBudget=F.Budget();if(!Check(F.RegisterComparison(JobIndex==1?TEXT("unresolved"):TEXT("normal")),TEXT("production registration accepts the supported finding")))return;
  Go(9011);return;
 }
 if(Stage==9011)
 {
  const int32 Before=WorkBudget;
  if(JobIndex==1&&F.PrinterProgress()<1)return;
  if(JobIndex==1)
  {
   if(!Check(F.CollectChart()&&!F.FilePacket(),TEXT("printed packet cannot be filed remotely from the console")))return;
   if(!FileAtBinder())return;
  }
  if(!Check(F.CurrentJob()->bPaid&&F.Budget()==Before+20,TEXT("one completed job receives exactly twenty dollars")))return;
  if(!Check(!F.RegisterComparison(TEXT("normal"))&&F.Budget()==Before+20,TEXT("repeated registration cannot duplicate payment")))return;
  if(!Restore(TEXT("paid-job")))return;F.Director->Stop();
  if(JobIndex<2){if(JobIndex==1)F.SetAnomaly(false);++JobIndex;Go(1);return;}Go(10);return;
 }
 if(Stage==10)
 {
  F.OpenArchive();Shot(TEXT("archive"));Go(11,.4);return;
 }
 if(Stage==11)
 {
  M->UI->Close(true);const int32 Day=F.Day(),Cash=F.Budget();
  if(!Check(F.Rest()&&F.Day()==Day+1&&F.Budget()==Cash,TEXT("rest advances a day without farming payments")))return;
  F.Director->Stop();++DayRuns;
  if(DayRuns<2){Go(11,.3);return;}
  if(!Check(F.Day()==3&&M->Story->ChapterKey==TEXT("freeform"),TEXT("third day continues without Story finale or chapter transition")))return;
  const int32 Photos=M->Night->Album->SaveData()->GetArrayField(TEXT("shots")).Num(),Left=M->Night->Album->Left();
  if(!Check(M->Night->Album->AddFilm(36)&&M->Night->Album->Left()==Left+36&&M->Night->Album->SaveData()->GetArrayField(TEXT("shots")).Num()==Photos,TEXT("loading fresh film preserves all archived photographs")))return;
  if(!Restore(TEXT("third-day")))return;F.Director->Stop();Go(20);return;
 }
 if(Stage==20)
 {
  M->UI->Close(true);F.Director->Stop();M->World->Enter(TEXT("road"));Go(40,.5);return;
 }
 if(Stage==40)
 {
  if(M->World->bBusy)return;
  // Use the existing road-parking identity and its authored, level diner lot.
  // Walking between this fixture and the shop remains an explicit placement.
  M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));
  const FVector P=F.Field->InteractionPoint(TEXT("ff:shop"));M->Player->Place(P.X,P.Z-1.3,0);
  Go(41,.6);return;
 }
 if(Stage==41)
 {
  if(!Check(F.Field->Purchase(TEXT("fuse")),TEXT("physical gas-station purchase creates a paid parcel")))return;
  if(!Check(F.Field->ParcelStage()==1,TEXT("purchased supplies remain at the shop until collected")))return;
  F.Field->Use(TEXT("ff:parcel"));
  if(!Check(F.Field->ParcelStage()==2,TEXT("parcel collected into a single authoritative carried state")))return;
  if(!Restore(TEXT("carried-purchase")))return;F.Director->Stop();Go(42,.4);return;
 }
 if(Stage==42)
 {
  if(!Check(F.Field->ParcelStage()==2,TEXT("physical purchase survives ordinary Continue without duplication")))return;
  const FVector P=F.Field->InteractionPoint(TEXT("ff:cargo"));M->Player->Place(P.X+1.4,P.Z,0);
  UE_LOG(LogTemp,Display,TEXT("S47 cargo fixture: player %.3f %.3f floor %.3f, rack %.3f %.3f %.3f, area %s vehicle %s"),M->Player->Pos.X,M->Player->Pos.Z,M->Player->FloorY,P.X,P.Y,P.Z,*M->World->Area,*M->World->ServicePlace());
  F.Field->Use(TEXT("ff:cargo"));
  if(!Check(F.Field->ParcelStage()==3,TEXT("carried purchase is secured as physical truck cargo")))return;
  if(!Restore(TEXT("loaded-cargo")))return;F.Director->Stop();Go(421,.4);return;
 }
 if(Stage==421)
 {
  if(!Check(F.Field->ParcelStage()==3,TEXT("loaded cargo survives ordinary Continue")))return;
  M->World->Enter(TEXT("saro"));Go(422,.4);return;
 }
 if(Stage==422)
 {
  if(M->World->bBusy)return;M->World->PlaceTruck(TEXT("saro"));
  const FVector P=F.Field->InteractionPoint(TEXT("ff:cargo"));M->Player->Place(P.X+1.4,P.Z,0);
  F.Field->Use(TEXT("ff:cargo"));
  if(!Check(F.Field->ParcelStage()==2,TEXT("truck cargo must be unloaded before physical bench delivery")))return;
  Go(43,.3);return;
 }
 if(Stage==43)
 {
  if(M->World->bBusy)return;
  const FVector P=F.Field->InteractionPoint(TEXT("ff:bench"));M->Player->Place(P.X,P.Z+1.5,0);
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  Snapshot=F.Field->Save();F.Field->Stop();Go(431,6.0);return;
 }
 if(Stage==431){Shot(TEXT("bench-before"));Go(432,.2);return;}
 if(Stage==432){F.Field->Begin(Snapshot);Go(433,2.0);return;}
 if(Stage==433){Shot(TEXT("bench-after"));Go(44,.2);return;}
 if(Stage==44)
 {
  const int32 Before=F.InventoryCount(TEXT("fuse"));F.Field->Use(TEXT("ff:bench"));
  if(!Check(F.Field->ParcelStage()==2&&F.InventoryCount(TEXT("fuse"))==Before,TEXT("calibration bench cannot unpack a supply parcel remotely from garage receiving")))return;
  const FVector Receiving=F.Field->InteractionPoint(TEXT("ff:delivery"));M->Player->Place(Receiving.X,Receiving.Z+1.5,0);F.Field->Use(TEXT("ff:delivery"));
  if(!Check(F.Field->ParcelStage()==0&&F.InventoryCount(TEXT("fuse"))==Before+1,TEXT("physical garage receiving unpacks one purchased part into persistent stock")))return;
  F.Field->Use(TEXT("ff:delivery"));M->UI->Close(true);
  if(!Check(F.InventoryCount(TEXT("fuse"))==Before+1,TEXT("repeated garage receiving use does not duplicate delivered supplies")))return;
  Shot(TEXT("service-bench"));Go(440,.3);return;
 }
 if(Stage==440)
 {
  // Explicit authorization/prepared-device fixture, not a naturally concluded case or field recording.
  // Exercise production station callbacks and the native checkout button, then restore the session.
  const auto Before=S47J::Read(S47J::Write(F.Save()));
  const auto Fixture=S47J::Read(S47J::Write(Before));
  auto Cases=S47J::Obj(Fixture,TEXT("cases"));auto Access=S47J::Strs(Cases,TEXT("access"));
  Access.AddUnique(TEXT("access.fieldlogger"));S47J::Set(Cases,TEXT("access"),Access);
  FS47FieldLogger Device;Device.Assembly=3;Device.Calibration=3;Device.bCalibrated=true;Device.Battery=FS47FieldLogger::Capacity;
  FS47FieldLogger::FTape Tape;Tape.Id=TEXT("fieldlogger01.tape1");Tape.Day=1;Tape.Start=100;Tape.End=220;Tape.Seconds=120;Tape.Offset=2.4;Tape.Levels={.2,.4};Tape.bRetrieved=true;
  Device.Tapes.Add(Tape);Device.Serial=1;
  S47J::Set(S47J::Obj(Fixture,TEXT("field")),TEXT("fieldlogger"),Device.Save());F.Begin(Fixture,F.Lethal());
  auto Place=[&](const TCHAR* Id){const FVector P=F.Field->InteractionPoint(Id);M->Player->Place(P.X,P.Z+1.5,0);M->UI->Close(true);};
  auto OriginalIntact=[&](){const auto& L=F.Field->FieldLogger();return L.Serial==1&&L.Tapes.Num()==1&&L.Tapes[0].Id==Tape.Id&&L.Tapes[0].Levels==Tape.Levels&&L.Tapes[0].Start==Tape.Start&&L.Tapes[0].End==Tape.End&&L.Tapes[0].Seconds==Tape.Seconds&&L.Tapes[0].Offset==Tape.Offset&&L.Tapes[0].bRetrieved&&!L.Tapes[0].bCompared;};
  Place(TEXT("ff:bench"));
  if(!Check(F.Field->LoggerAccess()&&!F.Field->LoggerAction(TEXT("take"))&&F.Field->ParcelStage()==0,TEXT("explicit authorized logger fixture cannot check out at calibration bench")))return;
  Place(TEXT("ff:depot"));
  if(!Check(!F.Field->LoggerAction(TEXT("calibrate"))&&!F.Field->LoggerAction(TEXT("tune"))&&!F.Field->LoggerAction(TEXT("compare")),TEXT("depot cannot calibrate, tune or compare field originals")))return;
  F.Field->Use(TEXT("ff:depot"));
  TArray<TSharedRef<SButton>> Buttons;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;
  if(Modal)PhotoButtons(Modal.ToSharedRef(),TEXT("Check out calibrated field logger"),Buttons);
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("parts storage exposes one enabled native equipment checkout button")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#else
  Finish(false,TEXT("native logger checkout test requires development build"));return;
#endif
  if(!Check(F.Field->ParcelStage()==2&&F.Field->ParcelItem()==TEXT("fieldlogger")&&!F.Field->LoggerAction(TEXT("take"))&&OriginalIntact(),TEXT("native checkout carries one logger; duplicate checkout preserves retained tape")))return;
  Place(TEXT("ff:bench"));F.Field->Use(TEXT("ff:bench"));
  if(!Check(F.Field->ParcelStage()==2&&OriginalIntact(),TEXT("wrong-station bench return is rejected without losing logger or original")))return;
  Place(TEXT("ff:depot"));F.Field->Use(TEXT("ff:depot"));
  if(!Check(F.Field->ParcelStage()==0&&F.Field->ParcelItem().IsEmpty()&&OriginalIntact(),TEXT("equipment checkout accepts return with original tape identity and samples intact")))return;
  F.Field->Use(TEXT("ff:depot"));M->UI->Close(true);
  const auto Returned=S47J::Read(S47J::Write(F.Field->Save()));F.Field->Begin(Returned);
  if(!Check(F.Field->ParcelStage()==0&&OriginalIntact(),TEXT("repeat depot use and JSON restore do not duplicate logger or alter original tape")))return;
  Place(TEXT("ff:bench"));
  if(!Check(F.Field->LoggerAction(TEXT("compare"))&&OriginalIntact(),TEXT("bench comparison opens but incomplete fixture tape cannot establish a valid comparison")))return;
  M->UI->Close(true);F.Begin(Before,F.Lethal());Go(441,.3);return;
 }
 if(Stage==441)
 {
  if(!F.Field->HasArrayFault())F.Field->TriggerServiceFault();
  F.Field->DiagnoseService();const FVector P=F.Field->InteractionPoint(TEXT("ff:service"));M->Player->Place(P.X-1.4,P.Z,0);Go(45,.4);return;
 }
 if(Stage==45)
 {
  if(!Check(F.Field->HasArrayFault()&&F.Field->ServiceStage()==2,TEXT("diagnosis identifies a real remotely unavailable antenna")))return;
  F.Field->Use(TEXT("ff:service"));F.Field->Use(TEXT("ff:service"));
  if(!Check(F.Field->ServiceStage()==4,TEXT("local isolation and actual hatch opening precede replacement")))return;
  const FVector P=F.Field->InteractionPoint(TEXT("ff:service"));
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  Go(451,6.0);return;
 }
 if(Stage==451){Shot(TEXT("service-open"));Go(452,.3);return;}
 if(Stage==452)
 {
  if(!Restore(TEXT("service-open")))return;F.Director->Stop();Go(46,.3);return;
 }
 if(Stage==46)
 {
  const FVector P=F.Field->InteractionPoint(TEXT("ff:service"));M->Player->Place(P.X-1.4,P.Z,0);
  if(!Check(F.Field->ServiceStage()==4,TEXT("open service hatch persists after disk/Flow restore")))return;
  F.Field->Use(TEXT("ff:service"));F.Field->Use(TEXT("ff:service"));F.Field->Use(TEXT("ff:service"));
  if(!Check(F.Field->ServiceStage()==7,TEXT("stock relay is mounted, hatch closed and physical local test started")))return;
  InitialAz=M->Web->DishAz(2);Go(47);return;
 }
 if(Stage==47)
 {
  if(F.Field->ServiceStage()!=9)return;
  if(!Check(!F.Field->HasArrayFault()&&M->Web->DishRemoteAvailable(2)&&FMath::Abs(M->Web->DishAz(2)-InitialAz)<=.1,TEXT("native local movement returns to its initial encoder angle and restores remote service")))return;
  if(!Restore(TEXT("service-complete")))return;F.Director->Stop();Go(48,.4);return;
 }
 if(Stage==48)
 {
  M->Player->Place(0,-2.1,0);Go(49,.4);return;
 }
 if(Stage==49)
 {
  // World transaction/repair fixtures are exercised separately in the same live world.
  if(!Check(F.Field!=nullptr,TEXT("physical supply world remains attached after multi-day Continue")))return;
  const auto Events=FS47FreeformDirector::EventIds();
  if(!Check(Events.Num()==19&&Events.Contains(TEXT("roomlight_return"))&&Events.Contains(TEXT("feeder_presence"))&&Events.Contains(TEXT("array_presence")),TEXT("director exposes sixteen authored events, the room-light incident and both physical service-site encounters")))return;
  F.TestAdvance(86400.);F.Director->Begin(nullptr);Go(21);return;
 }
 if(Stage==21)
 {
  auto Ids=FS47FreeformDirector::EventIds();
  Ids.Remove(TEXT("roomlight_return")); // Requires a real OFF input and departure; S47ReactiveLightTest owns that journey.
  // array_presence needs a healthy array; s03_failure leaves a fault, so stage the encounter first.
  Ids.Remove(TEXT("array_presence"));Ids.Insert(TEXT("array_presence"),FMath::Max(0,Ids.IndexOfByKey(FString(TEXT("s03_failure")))));
  if(EventIndex>=Ids.Num()){Go(30);return;}
  // Site encounters are anchored to their real apparatus: the remote feeder is on the road site,
  // every other event on the SARO yard. Load the correct site first, then stand at its anchor.
  const FString Site=Ids[EventIndex]==TEXT("feeder_presence")?TEXT("road"):TEXT("saro");
  if(M->Web->Area()!=Site){M->World->Enter(Site);Go(21,.6);return;}
  if(Ids[EventIndex]==TEXT("feeder_presence")||Ids[EventIndex]==TEXT("array_presence"))
  {const FVector P=F.Field->InteractionPoint(Ids[EventIndex]==TEXT("feeder_presence")?TEXT("ff:feeder"):TEXT("ff:service"));M->Player->Place(P.X,P.Z+4.,0);}
  else M->Player->Place(0,-1.2,PI);
  if(Ids[EventIndex]==TEXT("close_pass"))F.SpendTime(13*3600.); // native night presentation, not a naturally scheduled roll
  if(!F.Director->Force(Ids[EventIndex]))
  {
   UE_LOG(LogTemp,Warning,TEXT("S47 Freeform event fixture %s refused: area=%s arrayFault=%d individuals=%d nativeBody=%d active=%s"),*Ids[EventIndex],*M->Web->Area(),F.Field&&F.Field->HasArrayFault(),
    M->Night&&M->Night->Glimpses?M->Night->Glimpses->Individuals().Num():-1,M->Night&&M->Night->Glimpses&&M->Night->Glimpses->HasNativeBody(),*F.Director->ActiveEvent());
   Check(false,TEXT("explicit native event fixture starts: ")+Ids[EventIndex]);return;
  }
  bCarRoundTrip=false;PoseForEvent();
  Go(22,Ids[EventIndex]==TEXT("observer")||Ids[EventIndex]==TEXT("service_presence")?12.:Ids[EventIndex]==TEXT("close_pass")?3.:.6);return;
 }
 if(Stage==22)
 {
  if(!Check(!F.Director->ActiveEvent().IsEmpty(),TEXT("event owns a persisted running state")))return;
  const FString Id=F.Director->ActiveEvent();
  if(Id==TEXT("empty_car")||Id==TEXT("landing_trace")||Id==TEXT("response_lights")||Id==TEXT("flyover")||Id==TEXT("close_pass"))
   if(!Check(F.Director->PhysicalObjectPresented(),TEXT("event presents actual visible native render components: ")+Id))return;
  if(Id==TEXT("empty_car")&&!bCarRoundTrip){bCarRoundTrip=true;M->World->Enter(TEXT("diner"));Go(222,.4);return;}
  Shot(FString::Printf(TEXT("event-%02d"),EventIndex));
  if(F.Director->ActiveEvent()==TEXT("observer")||F.Director->ActiveEvent()==TEXT("service_presence"))
   if(!Check(M->Night&&M->Night->Glimpses&&!M->Night->Glimpses->Individuals().IsEmpty(),TEXT("controlled encounter has a real native individual after its warning")))return;
  if(F.Director->ActiveEvent()==TEXT("feeder_fault"))
  { M->World->Enter(TEXT("road"));Go(90,.4);return; }
  if(F.Director->ActiveEvent()==TEXT("wrong_source")||F.Director->ActiveEvent()==TEXT("time_disagreement")||F.Director->ActiveEvent()==TEXT("duplicate_print"))
  { Go(60);return; }
  if(F.Director->ActiveEvent()==TEXT("landing_trace"))
  {
   const FVector P=F.Director->EventAnchor();M->Player->Place(P.X-1.5,P.Z,0);
   const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   Go(23,.4);return;
  }
  Go(221,.4);return; // render the requested image before changing/restoring this event
 }
 if(Stage==222)
 {
  if(!Check(!F.Director->PhysicalObjectPresented(),TEXT("active physical incident is hidden and unqueryable outside its scene")))return;
  M->World->Enter(TEXT("saro"));Go(223,.4);return;
 }
 if(Stage==223){PoseForEvent();Go(22,.3);return;}
 if(Stage==221)
 {
  if(F.Director->ActiveEvent()==TEXT("empty_car"))
  {
   const FVector P=F.Director->EventAnchor();M->Player->Place(P.X-2.8,P.Z+1.,0);F.Director->Use(TEXT("ff:event-check"));
   if(!Check(F.HasFinding(TEXT("empty_car")),TEXT("physical empty-car inspection retains its independent field record")))return;
  }
  if(F.Director->ActiveEvent()==TEXT("extra_pulse")||F.Director->ActiveEvent()==TEXT("observer")||F.Director->ActiveEvent()==TEXT("service_presence"))
  {
   const FString Id=F.Director->ActiveEvent();if(!Restore(TEXT("active-event")))return;
   if(!Check(F.Director->ActiveEvent()==Id,TEXT("same event resumes from disk instead of rerolling")))return;
  }
  F.Director->Stop();F.Director->Begin(nullptr);++EventIndex;Go(21,.3);return;
 }
 if(Stage==23)
 {
  if(!Check(F.Director->PhysicalObjectPresented(),TEXT("field photograph requires the still-visible native sample")))return;
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();const FS47AlbumShot Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);
  if(!Check(!Photo.Id.IsEmpty()&&!Photo.Url.IsEmpty(),TEXT("native camera renders and retains the actual physical trace")))return;
  F.Director->Use(TEXT("ff:event-sample"));
  if(!Check(F.InventoryCount(TEXT("fragment"))==1&&F.HasFinding(TEXT("landing_trace")),TEXT("photographed trace becomes one retained physical sample")))return;
  const FVector WrongBench=F.Field->InteractionPoint(TEXT("ff:bench"));M->Player->Place(WrongBench.X,WrongBench.Z+1.5,0);
  if(!Check(!F.Field->AnalyzeSample(),TEXT("calibration bench cannot start analysis at the separate test lab")))return;
  const FVector P=F.Field->InteractionPoint(TEXT("ff:sample"));M->Player->Place(P.X,P.Z+1.5,0);Go(24,.3);return;
 }
 if(Stage==24)
 {
  if(!Check(F.Field->AnalyzeSample(),TEXT("physical sample analysis starts at the test-lab sample station")))return;
  Go(241,.7);return;
 }
 if(Stage==241)
 {
  if(!Restore(TEXT("sample-analysis-running")))return;
  if(!Check(!F.HasFinding(TEXT("sample-analysis"))&&F.InventoryCount(TEXT("fragment"))==0,TEXT("in-progress sample analysis restores without early completion or a duplicated loose sample")))return;
  Go(25);return;
 }
 if(Stage==25)
 {
  if(!F.HasFinding(TEXT("sample-analysis")))return;
  if(!Check(F.InventoryCount(TEXT("archived_fragment"))==1&&!F.Field->AnalyzeSample(),TEXT("timed analysis seals the original and cannot duplicate evidence")))return;
  F.Director->Stop();F.Director->Begin(nullptr);++EventIndex;Go(21,.3);return;
 }
 if(Stage==60)
 {
  int32 Pending=INDEX_NONE;
  for(int32 I=0;I<F.Jobs().Num();++I){const auto& Job=F.Jobs()[I];if(Job.Day==F.Day()&&Job.Slot==1&&Job.Stage<=J::Pointed){Pending=I;break;}}
  if(!Check(Pending!=INDEX_NONE&&F.SelectJob(Pending),TEXT("investigation selects its available pulsar or incident job through OPS")))return;
  const auto P=F.ConsoleProfile();if(!Check(P&&F.CurrentJob(),TEXT("instrument anomaly supplies a retained observation job")))return;
  auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
  if(!Check(F.PointSelected(),TEXT("specialized investigation commands its actual array pointing")))return;
  Go(61);return;
 }
 if(Stage==61){if(!F.StartAcquisition())return;Go(62);return;}
 if(Stage==62)
 {
  if(F.CurrentJob()->Stage!=J::Chart||F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart(),TEXT("specialized observation retains its physical original")))return;
  M->UI->Close(true);
  if(!Check(F.StartControl(),TEXT("retained original precedes its distinct measured instrument control")))return;
  Go(63);return;
 }
 if(Stage==63)
 {
  if(F.CurrentJob()->ControlAt<8)return;
  const auto* Job=F.CurrentJob();const FString Id=Job->Phenomenon;
  if(Id==TEXT("wrong_source"))
   if(!Check(Job->bBeamVerified&&Job->BeamAz.Num()==3&&Job->BeamAmplitude.Num()==3,TEXT("three real settled adjacent-beam samples precede off-source evidence")))return;
  if(Id==TEXT("time_disagreement"))
   if(!Check(Job->bClockMeasured&&FMath::Abs(Job->ReceiverClock-Job->ReferenceClock)>1&&FMath::Abs(Job->ControlReceiverClock-Job->ControlReferenceClock)>1,TEXT("independent clock readbacks establish the actual repeated disagreement")))return;
  if(Id==TEXT("duplicate_print")&&F.PrinterProgress()<1)return;
  if(Id==TEXT("duplicate_print"))
   if(!Check(F.CollectChart()&&F.CurrentJob()->bDuplicateCollected&&F.CurrentJob()->OriginalPage!=F.CurrentJob()->DuplicatePage,TEXT("a separate collected duplicate differs from the preserved original")))return;
  if(!Restore(TEXT("measured-")+Id))return;
  Go(64,.3);return;
 }
 if(Stage==64)
 {
  const FString Id=F.CurrentJob()->Phenomenon;
  if(!Check(F.CompareInstrument(),TEXT("measured distinct instrument comparison survives Continue: ")+Id))return;
  const FString Conclusion=Id==TEXT("wrong_source")?TEXT("off_source"):Id==TEXT("time_disagreement")?TEXT("timing_path"):TEXT("duplicate_mismatch");
  if(!Check(F.RegisterComparison(Conclusion),TEXT("supported instrument finding is registered: ")+Id))return;
  Go(641);return;
 }
 if(Stage==641)
 {
  const FString Id=F.CurrentJob()->Phenomenon;
  if(F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart(),TEXT("registered instrument packet finishes physical feed: ")+Id))return;
  if(!FileAtBinder())return;
  F.OpenArchive();Shot(TEXT("comparison-")+Id);Go(65,.3);return;
 }
 if(Stage==65)
 {
  M->UI->Close(true);F.Director->Stop();F.Director->Begin(nullptr);++EventIndex;Go(21,.3);return;
 }
 if(Stage==90)
 {
  if(M->World->bBusy)return;
  if(!Check(F.PowerFault()&&!M->Story->Rx.bPowered,TEXT("feeder incident cuts the actual receiver circuit before physical repair")))return;
  const FVector P=F.Field->InteractionPoint(TEXT("ff:feeder"));M->Player->Place(P.X-1.4,P.Z,0);Go(901,.6);return;
 }
 if(Stage==901)
 {
  F.Field->Use(TEXT("ff:feeder"));F.Field->Use(TEXT("ff:feeder"));F.Field->Use(TEXT("ff:feeder"));F.Field->Use(TEXT("ff:feeder"));
  if(!Check(S47J::Num(F.Field->Save(),TEXT("feeder"))==4,TEXT("physical feeder exchange reaches its enclosed timed test")))return;
  Go(91);return;
 }
 if(Stage==91)
 {
  if(F.PowerFault()||!M->Story->Rx.bPowered)return;
  if(!Check(!F.PowerFault()&&M->Story->Rx.bPowered,TEXT("physical feeder isolation, replacement and timed test restore actual receiver supply")))return;
  if(!Restore(TEXT("feeder-repaired")))return;M->World->Enter(TEXT("saro"));M->Player->Place(0,-1.2,PI);Go(92,.6);return;
 }
 if(Stage==92)
 {
  if(!Check(!F.PowerFault(),TEXT("repaired feeder remains powered after ordinary Continue")))return;
  F.Director->Stop();F.Director->Begin(nullptr);++EventIndex;Go(21,.3);return;
 }
 if(Stage==30)
 {
  const int32 Findings=F.FindingCount(),Cash=F.Budget();F.Recover(TEXT("isolated contact fixture"));
  if(!Check(!F.Dead()&&F.Budget()==Cash&&F.FindingCount()>=Findings,TEXT("standard local recovery retains secured evidence and budget")))return;
  if(!Restore(TEXT("local-recovery")))return;Go(31,.3);return;
 }
 if(Stage==31)
 {
  auto Saved=F.Save();S47J::Set(Saved,TEXT("lethal"),true);F.Begin(Saved,true);F.Recover(TEXT("isolated lethal fixture"));
  if(!Check(F.Dead(),TEXT("optional lethal consequence records death instead of local recovery")))return;
  // Restore the living snapshot; never save the dead session over it.
  Finish(true);return;
 }
}
