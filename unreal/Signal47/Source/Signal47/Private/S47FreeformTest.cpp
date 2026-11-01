#include "S47FreeformTest.h"
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
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Startup.h"
#include "S47Night.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Glimpses.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

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
 auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("diskFlowRestores"),Restores);S47J::Set(O,TEXT("scope"),bFieldResume?TEXT("field-and-events"):TEXT("full"));O->SetArrayField(TEXT("evidence"),Evidence);
 S47J::Set(O,TEXT("method"),TEXT("Isolated native Freeform integration: actual timed encoder settling, acquisition and audio; production workstation callbacks, full disk/Flow restores, accelerated day transitions and explicit event fixtures. Screenshots are native Vulkan. Fixture placement does not establish a natural town drive or human input/listening."));
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
  if(!Check(F.Field->ParcelStage()==0&&F.InventoryCount(TEXT("fuse"))==Before+1,TEXT("physical bench unpacks one purchased part into persistent stock")))return;
  F.Field->Use(TEXT("ff:bench"));M->UI->Close(true);
  if(!Check(F.InventoryCount(TEXT("fuse"))==Before+1,TEXT("repeated bench use does not duplicate delivered supplies")))return;
  Shot(TEXT("service-bench"));Go(441,.3);return;
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
  if(!Check(FS47FreeformDirector::EventIds().Num()==16,TEXT("director exposes the sixteen authored event families")))return;
  F.TestAdvance(86400.);F.Director->Begin(nullptr);Go(21);return;
 }
 if(Stage==21)
 {
  const auto Ids=FS47FreeformDirector::EventIds();
  if(EventIndex>=Ids.Num()){Go(30);return;}
  M->Player->Place(0,-1.2,PI);
  if(Ids[EventIndex]==TEXT("close_pass"))F.SpendTime(13*3600.); // native night presentation, not a naturally scheduled roll
  if(!Check(F.Director->Force(Ids[EventIndex]),TEXT("explicit native event fixture starts: ")+Ids[EventIndex]))return;
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
  if(EventIndex==0||EventIndex==3||EventIndex==5)
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
  const FVector P=F.Field->InteractionPoint(TEXT("ff:bench"));M->Player->Place(P.X,P.Z+1.5,0);Go(24,.3);return;
 }
 if(Stage==24)
 {
  if(!Check(F.Field->AnalyzeSample(),TEXT("physical sample analysis starts at the service bench")))return;
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
