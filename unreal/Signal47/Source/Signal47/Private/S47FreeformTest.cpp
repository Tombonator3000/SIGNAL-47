#include "S47FreeformTest.h"
#include "S47FreeformAudioDesk.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
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
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("diskFlowRestores"),Restores);S47J::Set(O,TEXT("scope"),FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTripHoldTest"))?TEXT("actual-trip-work-hold"):FParse::Param(FCommandLine::Get(),TEXT("S47PhotoEconomyTest"))?TEXT("photo-economy-rendered-fixture"):bFieldResume?TEXT("field-and-events"):TEXT("full"));O->SetArrayField(TEXT("evidence"),Evidence);
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
 if(!M||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Flow||M->Flow->bStarting||S47Startup::Blocking())return;
 Age+=FMath::Min(Dt,.2);if(Age>900){Finish(false,TEXT("bounded integration timeout"));return;}if(Hold>0){Hold-=Dt;return;}
 auto& F=*M->Freeform;using J=FS47FreeformSession::EJobStage;
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
   if(F.CurrentJob()->Stage!=J::Chart)return;
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
  if(F.CurrentJob()->Stage!=J::Chart)return;
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
  const int32 Before=F.Budget();if(!Check(F.RegisterComparison(JobIndex==1?TEXT("unresolved"):TEXT("normal")),TEXT("production registration accepts the supported finding")))return;
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
  const FVector P=F.Field->InteractionPoint(TEXT("ff:shop"));M->Player->Place(P.X-1.4,P.Z,0);
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
  if(F.CurrentJob()->Stage!=J::Chart)return;
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
  if(!Check(F.RegisterComparison(Conclusion)&&F.CollectChart(),TEXT("supported instrument finding is registered and physically collected: ")+Id))return;
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
