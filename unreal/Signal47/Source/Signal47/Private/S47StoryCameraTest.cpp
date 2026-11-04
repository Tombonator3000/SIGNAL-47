#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Chapter1.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47FieldCamera.h"
#include "S47Startup.h"
#include "Misc/FileHelper.h"
#include "S47HUD.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

void FS47FreeformTest::StepStoryCamera(double Dt)
{
 if(bStoryAccessReady)return; // EastWing DriveTest owns input and final result after handoff.
 Age+=FMath::Min(Dt,.2);if(Age>180){Finish(false,TEXT("bounded Story camera regression timeout"));return;}
 if(Hold>0){Hold-=Dt;return;}
 if(!M||!M->Flow)return;
 if(Stage==8700)
 {
  if(M->Mode!=US47Main::EMode::Title)return;
  if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("title Continue callback dispatched; native menu input not claimed")))return;
  Go(8701,1);return;
 }
 if(M->Flow->bStarting||S47Startup::Blocking()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Story||!M->Inter||!M->UI)return;
 auto* C=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
 if(!C||M->IsFreeform()){Check(false,TEXT("Story controller and session mode retained"));return;}
 auto SaveAndTitle=[&](int32 Next)
 {
  M->UI->Close(true);StoryCameraSnapshot=S47J::Read(S47J::Write(C->SaveState()));StoryCameraResume=Next;StoryCameraClock=M->Story->Clock();
  FString Error;const auto Written=M->Flow->WriteSave(TEXT("manual"),0,Error);
  FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Choice=Reader.ContinueSave();const auto Disk=Choice?Reader.Load(Choice->Id):nullptr;
  if(!Check(Written&&Choice&&Disk&&Written->Id==Choice->Id&&S47J::Str(Disk,TEXT("sessionMode"))==TEXT("story"),TEXT("quarantined WriteSave API and fresh reader select actual Story snapshot: ")+Error))return;
  FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/FString::Printf(TEXT("story-camera-%d.json"),Next)));
  M->Flow->ToTitle();Go(8700,.5);
 };
 auto Aim=[&](double X,double Z,const FVector& Target)
 {
  M->Player->Place(X,Z,0);const FVector D=Target-FVector(X,M->Player->FloorY+M->Player->Eye,Z);
  M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 };
 if(Stage==8690)
 {
  const auto Camera=M->Web->Obj(TEXT("room.fieldCamera"));
  if(!Check(Camera&&Camera.WorldPosition().Equals(FVector(5.8,.97,.66),.001)&&!S47J::Bool(C->SaveState(),TEXT("camera")),TEXT("chapter1 start fixture restores original unowned Story camera after Freeform")))return;
  // Metadata fixture only. No chapter/stage/film/development state is injected.
  M->Flow->Saves=FS47SaveStore(Root/TEXT("StorySaveGames"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);
  SaveAndTitle(8691);return;
 }
 if(Stage==8701)
 {
  auto Current=C->SaveState();auto Expected=S47J::Read(S47J::Write(StoryCameraSnapshot));
  // Event origin is exact; the live Story clock may advance during the startup settle.
  const double SavedClock=S47J::Num(Expected,TEXT("eventClock"));const double CurrentClock=S47J::Num(Current,TEXT("eventClock"));
  Current->RemoveField(TEXT("eventClock"));Expected->RemoveField(TEXT("eventClock"));
  if(!Check(FJsonValue::CompareEqual(FJsonValueObject(Current),FJsonValueObject(Expected))&&CurrentClock==SavedClock&&M->Story->Clock()>=StoryCameraClock&&M->Story->Clock()-StoryCameraClock<5&&M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("title/disk Continue retains exact Story progression, camera and original; clock stays near saved time")))return;
  ++Restores;
  if(StoryCameraResume==8691&&FParse::Param(FCommandLine::Get(),TEXT("S47StoryAccessAfterFreeform")))
  {
   if(!Check(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingDriveTest")),TEXT("Story access handoff requires existing EastWingDriveTest")))return;
   M->UI->Close(true);M->Player->TestMove.Reset();
   auto Proof=S47J::New();S47J::Set(Proof,TEXT("status"),TEXT("READY_FOR_ACCESS_TEST"));S47J::Set(Proof,TEXT("storyRestores"),1);S47J::Set(Proof,TEXT("totalDiskFlowRestores"),Restores);
   S47J::Set(Proof,TEXT("sourceFreeformCamera"),FParse::Param(FCommandLine::Get(),TEXT("S47StoryCameraUnowned"))?TEXT("unowned"):TEXT("owned"));
   S47J::Set(Proof,TEXT("method"),TEXT("Freeform camera source followed by chapter1 start fixture and verified quarantined WriteSave/title Continue; camera progression intentionally stops before Story pickup. EastWingDriveTest produces final access result."));
   if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Proof,true),*(Root/TEXT("story-access-handoff.json"))),TEXT("handoff evidence written before access driver starts")))return;
   bStoryAccessReady=true;return;
  }
  Go(StoryCameraResume,.3);return;
 }
 if(Stage==8691)
 {
  M->UI->Close(true);const auto* It=M->Inter->Get(TEXT("fieldCamera"));
  if(!Check(It&&It->Object&&!M->Inter->LabelOf(TEXT("fieldCamera")).IsEmpty(),TEXT("Story camera remains physically available after pre-pickup Continue")))return;
  Aim(4.7,.66,It->Object.WorldBox().GetCenter());Go(8692,.4);return;
 }
 if(Stage==8692)
 {
  const auto Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("fieldCamera")&&bool(M->Player->OnUse),TEXT("actual Story shelf camera is selectable by eye ray")))return;
  M->Player->OnUse(Pick);const auto State=C->SaveState();
  if(!Check(S47J::Bool(State,TEXT("camera"))&&S47J::Str(State,TEXT("stage"))==TEXT("motor-log")&&M->Inter->LabelOf(TEXT("fieldCamera")).IsEmpty(),TEXT("physical Story pickup advances only to motor-log and removes duplicate pickup")))return;
  SaveAndTitle(8693);return;
 }
 if(Stage==8693)
 {
  const auto* It=M->Inter->Get(TEXT("motorBus"));
  if(!Check(It&&It->Object,TEXT("Story motor log has physical interaction object")))return;
  const auto T=It->Object.WorldBox().GetCenter();Aim(T.X,T.Z+1,T);Go(8694,.4);return;
 }
 if(Stage==8694)
 {
  const auto Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("motorBus")&&bool(M->Player->OnUse),TEXT("Story motor log is selected by physical ray")))return;
  M->Player->OnUse(Pick);M->UI->Close(true);
  if(!Check(S47J::Bool(C->SaveState(),TEXT("log")),TEXT("production motor-log use unlocks first exposure")))return;
  // Existing chapter1 apron fixture; target is the S-03 upper structure, not a fabricated image.
  Aim(10.4,-11.6,FVector(9.5,11.16,-42));M->Player->Pitch-=.06;Go(8695,.4);return;
 }
 if(Stage==8695)
 {
  C->ToggleCamera();Go(8696,.4);return;
 }
 if(Stage==8696)
 {
  if(!Check(!S47J::Obj(C->SaveState(),TEXT("f1")),TEXT("no injected Story first exposure before shutter")))return;
  if(!Check(M->FieldCamera&&M->FieldCamera->bRaised,TEXT("Story camera is actually raised before native shutter")))return;
  M->FieldCamera->Sync();
  if(!Check(M->FieldCamera->PhotoLoc.Equals(M->Player->EyeUE(),.01)&&M->FieldCamera->PhotoRot.Vector().Equals(M->Player->ForwardUE(),.0001),TEXT("settled photograph pose matches actual viewport eye and direction")))return;
  C->Shutter();const auto Photo=S47J::Obj(C->SaveState(),TEXT("f1"));
  if(!Check(Photo&&!S47J::Str(Photo,TEXT("url")).IsEmpty(),TEXT("native Story shutter retains actual first image after Freeform and Continue")))return;
  if(M->FieldCamera)M->FieldCamera->Raise(false);M->UI->Close(true);SaveAndTitle(8697);return;
 }
 if(Stage==8697)
 {
  if(!Check(S47J::Obj(C->SaveState(),TEXT("f1"))&&S47J::Num(C->SaveState(),TEXT("dev1"))==0,TEXT("actual original survives Continue without injected development")))return;
  Go(8698,.3);return;
 }
 // Placement is an explicit harness fixture, not a walked route. Every use still
 // requires the actual eye ray and production Walker OnUse handler.
 auto Prepare=[&](const FString& Id)
 {
  M->UI->Close(true);const auto* It=M->Inter->Get(Id);
  if(!Check(It&&It->Object,TEXT("physical Story lab object: ")+Id))return false;
  const auto T=It->Object.WorldBox().GetCenter();Aim(T.X,T.Z+1,T);return true;
 };
 auto Use=[&](const FString& Id)
 {
  if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==Id&&bool(M->Player->OnUse),TEXT("actual Story lab eye ray: ")+Id))return false;
  M->Player->OnUse(Id);return true;
 };
 auto Button=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;
  TFunction<bool(TSharedRef<SWidget>)> Has=[&](TSharedRef<SWidget> W){if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label)return true;auto* Kids=W->GetChildren();for(int I=0;Kids&&I<Kids->Num();++I)if(Has(Kids->GetChildAt(I)))return true;return false;};
  TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> W){if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("SButton")&&Has(W))Found.Add(StaticCastSharedRef<SButton>(W));auto* Kids=W->GetChildren();for(int I=0;Kids&&I<Kids->Num();++I)Visit(Kids->GetChildAt(I));};
  if(M->Hud&&M->Hud->ModalWidget())Visit(M->Hud->ModalWidget().ToSharedRef());
  if(!Check(Found.Num()==1&&Found[0]->IsEnabled(),TEXT("unique enabled Story wet-bench button: ")+Label))return false;
#if !UE_BUILD_SHIPPING
  Found[0]->SimulateClick();return true;
#else
  return Check(false,TEXT("native button harness requires development build"));
#endif
 };
 if(Stage==8698)
 {
  if(M->Web->IsDoorOpen(TEXT("labDoor"))){if(!Prepare(TEXT("labDoor")))return;Go(86970,.4);}
  else Go(86971,.3);return;
 }
 if(Stage==86970){if(!Use(TEXT("labDoor")))return;Go(86971,.8);return;}
 if(Stage==86971)
 {
  if(!Check(!M->Web->IsDoorOpen(TEXT("labDoor")),TEXT("real darkroom door is closed before processing"))||!Prepare(TEXT("wetBench")))return;
  Go(86972,.4);return;
 }
 if(Stage==86972){if(!Use(TEXT("wetBench")))return;Go(86973,.3);return;}
 if(Stage==86973)
 {
  if(!Button(TEXT("LOAD SEALED FILM INTO TANK")))return;
  if(!Check(S47J::Int(C->SaveState(),TEXT("dev1"))==0,TEXT("loading does not skip actual tank timer")))return;
  Go(86974,.3);return;
 }
 if(Stage==86974)
 {
  if(S47J::Int(C->SaveState(),TEXT("dev1"))==0)return;
  if(!Check(S47J::Int(C->SaveState(),TEXT("dev1"))==1&&!M->Web->IsDoorOpen(TEXT("labDoor")),TEXT("real tank timer finishes with door closed"))||!Use(TEXT("wetBench")))return;
  Go(86975,.3);return;
 }
 if(Stage==86975)
 {
  if(!Button(TEXT("TRANSFER CONTACT PRINT TO FIXER")))return;
  if(!Check(S47J::Int(C->SaveState(),TEXT("dev1"))==1,TEXT("transfer does not skip actual fixer timer")))return;
  Go(86976,.3);return;
 }
 if(Stage==86976)
 {
  if(S47J::Int(C->SaveState(),TEXT("dev1"))==1)return;
  if(!Check(S47J::Int(C->SaveState(),TEXT("dev1"))==2&&!M->Web->IsDoorOpen(TEXT("labDoor")),TEXT("real fixer timer stabilizes print with door closed"))||!Use(TEXT("wetBench")))return;
  Go(86977,.3);return;
 }
 if(Stage==86977){if(!Button(TEXT("COLLECT AND EXAMINE PRINT")))return;Go(86978,.3);return;}
 if(Stage==86978)
 {
  const auto Now=C->SaveState();const auto Original=S47J::Obj(StoryCameraSnapshot,TEXT("f1"));const auto Current=S47J::Obj(Now,TEXT("f1"));
  if(!Check(Original&&Current&&FJsonValue::CompareEqual(FJsonValueObject(Original),FJsonValueObject(Current))&&S47J::Int(Now,TEXT("dev1"))==3&&S47J::Int(Now,TEXT("fog1"))==0&&S47J::Str(Now,TEXT("stage"))==TEXT("interpret-first"),TEXT("actual collection develops unchanged original with no fog or injected progression")))return;
  M->UI->ClearToasts();Shot(TEXT("story-developed-original"));Go(8699,.8);return;
 }
 if(Stage==8699){SaveAndTitle(86979);return;}
 if(Stage==86979)
 {
  if(!Check(S47J::Obj(C->SaveState(),TEXT("f1"))&&S47J::Int(C->SaveState(),TEXT("dev1"))==3&&S47J::Str(C->SaveState(),TEXT("stage"))==TEXT("interpret-first"),TEXT("developed original and next Story objective survive fresh-reader title Continue")))return;
  Finish(true,TEXT("Story pickup, shutter, actual closed-door tank/fixer timers and collection; four quarantined WriteSave/title Continue rounds. Placement and native callbacks, not human walking/menu input or separate process restart."));return;
 }

}
