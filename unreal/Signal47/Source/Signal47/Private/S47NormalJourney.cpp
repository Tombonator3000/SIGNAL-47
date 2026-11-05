#include "S47NormalJourney.h"
#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47DuskOrb.h"
#include "S47RoadShape.h"
#include "S47Saves.h"
#include "Input/Events.h"
#include "HAL/PlatformMisc.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47FreeformDirector.h"
#include "S47FreeformAudioDesk.h"
#include "S47Audio.h"
#include "S47WebAudio.h"
#include "S47Story.h"
#include "S47Night.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47HUD.h"
#include "S47UI.h"
#include "S47Keys.h"
#include "S47Three.h"
#include "S47Startup.h"
#include "S47Props.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
namespace
{
 FString Hash(const FString& Text){FTCHARToUTF8 B(*Text);uint8 Out[20];FSHA1::HashBuffer(B.Get(),B.Length(),Out);return BytesToHex(Out,20);}
 bool Text(const TSharedRef<SWidget>& W,const FString& Label)
 {
  if(!W->GetVisibility().IsVisible())return false;
  if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label)return true;
  auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(Text(C->GetChildAt(I),Label))return true;return false;
 }
 void Buttons(const TSharedRef<SWidget>& W,const FString& Label,TArray<TSharedRef<SButton>>& Out)
 {if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("SButton")&&Text(W,Label))Out.AddUnique(StaticCastSharedRef<SButton>(W));auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)Buttons(C->GetChildAt(I),Label,Out);}
 void Sliders(const TSharedRef<SWidget>& W,TArray<TSharedRef<SSlider>>& Out)
 {if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("SSlider"))Out.AddUnique(StaticCastSharedRef<SSlider>(W));auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)Sliders(C->GetChildAt(I),Out);}
 void Knobs(const TSharedRef<SWidget>& W,const FString& Label,TArray<TSharedRef<SSlider>>& Out)
 {if(W->GetTypeAsString()==TEXT("SVerticalBox")&&Text(W,Label)){TArray<TSharedRef<SSlider>> Local;Sliders(W,Local);if(Local.Num()==1)Out.AddUnique(Local[0]);}auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)Knobs(C->GetChildAt(I),Label,Out);}
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;int Phase=0,Stage=0,Knob=0,Checks=0;bool Done=false;double Age=0,At=0,SaveAt=0,TravelAt=0;FString Root,User,Photo,Job;TMap<FString,FString> PreservedSaves;int Waits=0;S47J::FObj Base=S47J::New();TArray<TSharedPtr<FJsonValue>> Evidence;TWeakPtr<SSlider> Locked;bool ChangedDials=false;FString CaptureJob,Original,Control,Document;TArray<double> Ticks;int LoadingSeen=-1,LoadingChecked=-1;
  bool Finish(bool Good,const FString& Why)
  {
   auto R=S47J::New();S47J::Set(R,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(R,TEXT("reason"),Why);S47J::Set(R,TEXT("phase"),Phase);S47J::Set(R,TEXT("stage"),Stage);S47J::Set(R,TEXT("checks"),Checks);
   S47J::Set(R,TEXT("method"),TEXT("Normal case from title input, actual save menu and separate process per phase. Explicit player placement, synthetic Slate button/slider input, actual motors/recording/photo/lab/purchases; production DriveToDiner/DriveBack callbacks with autopilot. No human walking/driving claim, phases4/5 natural dusk invitation and once-only event-photo credit, no injected job/stock/cash/image state."));if(Phase>=6)S47J::Set(R,TEXT("method"),TEXT("Ordinary case1, title Continue and manual Save Case across separate processes. Explicit existing pulsar selection/anomaly/dial/approach fixtures; actual pointing, acquisition, printer eye ray, control, source rendering and transport. No fabricated job, elapsed, recipe, sample completion or listening coverage; no natural anomaly discovery or human listening claim."));R->SetArrayField(TEXT("evidence"),Evidence);
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(R,true),*(Root/FString::Printf(TEXT("phase-%d.json"),Phase)));
   UE_LOG(LogTemp,Display,TEXT("S47 normal journey %s phase%d stage%d: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Phase,Stage,*Why);Done=true;FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  bool Check(bool Good,const FString& Why){++Checks;auto E=S47J::New();S47J::Set(E,TEXT("pass"),Good);S47J::Set(E,TEXT("check"),Why);Evidence.Add(MakeShared<FJsonValueObject>(E));if(!Good)Finish(false,Why);return Good;}
  void Go(int Next){Stage=Next;At=Age;UE_LOG(LogTemp,Display,TEXT("S47 normal journey phase%d stage%d at%.1fs"),Phase,Stage,Age);}
  TArray<TSharedRef<SWidget>> Roots()
  {
   TArray<TSharedRef<SWidget>> Out;TSet<const SWidget*> Seen;auto M=Main.Get();if(M->Hud&&M->Hud->ModalWidget()){auto W=M->Hud->ModalWidget();Seen.Add(W.Get());Out.Add(W.ToSharedRef());}
   if(M->Freeform->TerminalOnGlass())FSlateApplication::Get().ForEachUser([&](FSlateUser& U){auto W=U.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;if(!Seen.Contains(W.Get())){Seen.Add(W.Get());Out.Add(W.ToSharedRef());}},true);return Out;
  }
  bool Enabled(const FString& Label){TArray<TSharedRef<SButton>> B;for(auto W:Roots())Buttons(W,Label,B);return B.Num()==1&&B[0]->IsEnabled();}
  bool Click(const FString& Label)
  {
   TArray<TSharedRef<SButton>> Found;for(auto W:Roots())Buttons(W,Label,Found);
   if(!Check(Found.Num()==1&&Found[0]->IsEnabled(),TEXT("unique enabled native button: ")+Label))return false;
#if !UE_BUILD_SHIPPING
   Found[0]->SimulateClick();return true;
#else
   return Check(false,TEXT("development binary required for native Slate button activation"));
#endif
  }
  void Aim(const FVector& P){auto M=Main.Get();const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
  bool Place(const FString& Id,FVector Offset=FVector(0,0,1))
  {
   auto M=Main.Get();M->UI->Close(true);const auto* I=M->Inter->Get(Id);const auto* Item=M->Web->Item(Id);
   if(!Check((I&&I->Object)||Item,TEXT("physical object for placement: ")+Id))return false;
   FVector P=Id.StartsWith(TEXT("ff:"))?M->Freeform->Field->InteractionPoint(Id):(I&&I->Object?I->Object.WorldPosition():S47Three::ToThree(M->Web->PropLocation(Id)));if(Id==TEXT("ff:delivery")&&I&&I->Object){const FBox B=I->Object.WorldBox();if(!Check(B.IsValid,TEXT("receiving bench has measured native bounds")))return false;P=B.GetCenter();P.Y=B.Max.Y-.015;UE_LOG(LogTemp,Display,TEXT("S47 receiving bench bounds=%s target=%s"),*B.ToString(),*P.ToString());}M->Player->Place(P.X+Offset.X,P.Z+Offset.Z,0);Aim(P);return true;
  }
  bool Use(const FString& Id){auto M=Main.Get();const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());if(!Check(Picked==Id&&bool(M->Player->OnUse),TEXT("actual use ray: ")+Id+TEXT(" / picked ")+Picked+TEXT(" / player ")+M->Player->Pos.ToString()))return false;M->Player->OnUse(Id);return true;}
  const FS47AlbumShot* Shot(){return Main->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==Photo;});}
  bool Retained()
  {
   auto M=Main.Get();auto& F=*M->Freeform;auto* J=F.Jobs().FindByPredicate([&](const FS47FreeformSession::FJob& X){return X.Id==Job;});
   return Check(J&&J->bPaid&&J->Stage==FS47FreeformSession::EJobStage::Filed&&Hash(J->OriginalPage)==S47J::Str(Base,TEXT("originalHash"))&&J->OriginalStamp==S47J::Str(Base,TEXT("stamp"))&&Shot()&&!Shot()->bUnavailable&&Shot()->Pose.IsSet()&&Shot()->Pose->Valid()&&Shot()->Url.StartsWith(TEXT("data:image/"))&&Hash(Shot()->Url)==S47J::Str(Base,TEXT("photoHash"))&&M->Night->Album->Left()==S47J::Int(Base,TEXT("film"))-(S47J::Str(Base,TEXT("eventPhoto")).IsEmpty()?0:1),TEXT("same paid routine document and original photograph survive normal Continue"));
  }
  const FS47FreeformSession::FJob* CaptureCard()const
  {return Main->Freeform->Jobs().FindByPredicate([&](const auto& J){return J.Id==CaptureJob;});}
  bool CaptureRetained()
  {
   const auto* J=CaptureCard();
   return Check(S47J::Obj(Base,TEXT("captureOriginal")).IsValid()&&S47J::Obj(Base,TEXT("captureControl")).IsValid()&&J&&J->CaptureOriginal.Playable()&&J->CaptureControl.Playable()&&FJsonValue::CompareEqual(FJsonValueObject(J->CaptureOriginal.Save()),FJsonValueObject(S47J::Obj(Base,TEXT("captureOriginal"))))&&FJsonValue::CompareEqual(FJsonValueObject(J->CaptureControl.Save()),FJsonValueObject(S47J::Obj(Base,TEXT("captureControl"))))&&Hash(J->OriginalPage)==S47J::Str(Base,TEXT("captureDocumentHash"))&&J->OriginalStamp==S47J::Str(Base,TEXT("captureStamp"))&&J->TraceTicks==S47J::Nums(Base,TEXT("captureTicks"))&&J->OriginalCovered==S47J::Num(Base,TEXT("captureOriginalCovered"))&&J->ControlCovered==S47J::Num(Base,TEXT("captureControlCovered")),TEXT("real v3 recipes, settings/frames, original/stamp, ticks and actual coverage exactly match manual checkpoint"));
  }
  bool CaptureHealth()
  {
   auto M=Main.Get();auto& F=*M->Freeform;
   return Check(M->Audio&&M->Audio->Ctx&&M->Audio->Ctx->State()==TEXT("running")&&!FParse::Param(FCommandLine::Get(),TEXT("NoSound"))&&F.AvailableDishCount()==27&&!F.PowerFault()&&!F.Field->HasArrayFault(),TEXT("native audio clock available and array27/27 after the declared wear fixture, without adding service results"));
  }
  int SaveSlot()const{return Phase<=3?Phase-1:Phase-4;}
  bool VisibleText(const FString& Label){for(auto R:Roots())if(Text(R,Label))return true;return false;}
  const FS47AlbumShot* EventShot(){const FString Id=S47J::Str(Base,TEXT("eventPhoto"));return Main->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==Id;});}
  bool EventRetained(bool Paid)
  {
   auto& F=*Main->Freeform;const auto* P=EventShot();const FString Id=S47J::Str(Base,TEXT("eventClaim"));
   const auto* C=F.Director->PhotoClaims().FindByPredicate([&](const auto& X){return X.Id==Id;});
   return Check(P&&P->Id!=Photo&&!P->bUnavailable&&P->IsDeveloped()&&P->Pose.IsSet()&&P->Pose->Valid()&&P->Pose->Scene==TEXT("saro")&&Hash(P->Url)==S47J::Str(Base,TEXT("eventHash"))&&P->Clock==S47J::Num(Base,TEXT("eventClock"))&&C&&C->PhotoId==P->Id&&C->bPaid==Paid&&C->Reward==20,TEXT("separate authentic developed event original and exact incident/payment retained"))&&Retained();
  }
  void SaveMenu()
  {
   auto M=Main.Get();M->UI->Close(true);SaveAt=S47Saves::NowMs();
   if(Phase<=3){S47Keys::TestPresses(TEXT("Escape,Wait,Down,Enter,Wait,Enter"),.5f);Go(900);return;}
   PreservedSaves.Reset();bool Selected=false;int Manuals=0;
   for(const auto& S:M->Flow->Saves.List(1))if(S.Kind==TEXT("manual")){++Manuals;if(S.Slot==SaveSlot())Selected=true;else
   {
    const auto Disk=M->Flow->Saves.Load(S.Id);
    if(!Check(Disk.IsValid(),TEXT("non-target manual save readable before overwrite: ")+S.Id))return;
    PreservedSaves.Add(S.Id,Hash(S47J::Write(Disk)));
   }}
   if(!Check(Manuals==3&&Selected,TEXT("three occupied manual slots; explicit ordinary overwrite required")))return;
   auto Proof=S47J::New();for(const auto& Pair:PreservedSaves)S47J::Set(Proof,*Pair.Key,Pair.Value);
   S47J::Set(Proof,TEXT("targetSlot"),SaveSlot());FFileHelper::SaveStringToFile(S47J::Write(Proof,true),*(Root/FString::Printf(TEXT("phase-%d-before-overwrite-hashes.json"),Phase)));
   S47Keys::TestPresses(TEXT("Escape,Wait,Down,Enter"),.5f);Go(890);
  }
  bool Tune()
  {
   auto M=Main.Get();auto P=M->Freeform->ConsoleProfile();if(!P){Check(false,TEXT("retained job has receiver profile"));return false;}
   const TCHAR* Names[]={TEXT("FREQUENCY"),TEXT("GAIN"),TEXT("BANDWIDTH")};const double Want[]={P->F,(P->Gain[0]+P->Gain[1])/2,(P->Bw[0]+P->Bw[1])/2};const double Have[]={M->Story->Rx.Frequency,M->Story->Rx.Gain,M->Story->Rx.Bandwidth};const double Tolerance[]={.00051,.51,.51};
   if(Knob>=3)return true;TArray<TSharedRef<SSlider>> Found;for(auto W:Roots())Knobs(W,Names[Knob],Found);if(Found.Num()!=1){Check(false,TEXT("unique actual receiver slider"));return false;}auto S=Found[0];
   if(Locked.Pin()!=S){S->OnKeyDown(S->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));Locked=S;}
   if(FMath::Abs(Have[Knob]-Want[Knob])<=Tolerance[Knob]){S->OnKeyDown(S->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));Locked.Reset();++Knob;return false;}
   S->OnNavigation(S->GetCachedGeometry(),FNavigationEvent(FModifierKeysState(),0,Have[Knob]<Want[Knob]?EUINavigation::Right:EUINavigation::Left,ENavigationGenesis::Keyboard));return false;
  }
  bool AtDiner(const FVector& P) const
 {
  auto M=Main.Get();for(const FString& Path:M->Web->PathsUnder(TEXT("world.town.zones[")))if(M->Web->Rect(Path).Contains(P.X,P.Z))return true;
  const auto& D=S47RoadShape::DINER();const double X=8000+D.X,Z=D.Z;
  return P.X>X-1&&P.X<X+20&&P.Z>Z-10&&P.Z<Z+20; // same lot margin as production AtDiner
 }
 bool Travel(const FString& To)
  {
   auto W=Main->World.Get();if(W->bBusy)return false;
   if(W->bDriving)
   {
    // Autopilot brakes at the route endpoint; only attempt to get out after
    // sufficient actual travel, at destination, and at its stopped endpoint.
    if(Age-TravelAt>10&&FMath::Abs(W->DrivingSpeed())<.1&&W->CanGetOut())
    { // CanGetOut alone permits roadside stops; confirm destination by native world area.
     const auto* D=W->CabDrive();if(D){const FVector P=D->Pos;const bool AtDestination=To==TEXT("diner")?AtDiner(P):(W->Area==TEXT("saro")&&P.X>=6.6&&P.X<=13.0&&P.Z>=3.4&&P.Z<=12.4);if(AtDestination)W->GetOutHere();}
    }
    return false;
   }
   return W->Area==To&&W->TruckAt==To;
  }
  bool Tick(float Dt)
  {
   if(Done)return false;Age+=FMath::Max(0.f,Dt);auto M=Main.Get();if(!M)return Finish(false,TEXT("main lost"));
   if(Age>1800||Age-At>600)return Finish(false,TEXT("bounded normal-journey timeout; no gate bypass attempted"));
   if(!M->Flow||!M->UI||!M->Player||!M->World||!M->Web||!M->Web->IsLoaded()||S47Startup::Blocking())return true;
   if(Age-At<.4&&!(Stage>=601&&Stage<=608)&&Stage!=702&&Stage!=703)return true;
   if(Stage==0)
   {
    if(Age<5)return true;
    if(!Check(M->Mode==US47Main::EMode::Title,TEXT("ordinary title before normal case start/Continue")))return false;
    if(Phase==1){if(!Check(M->Flow->Saves.List().IsEmpty(),TEXT("new journey requires empty isolated profile")))return false;S47Keys::TestPresses(TEXT("Down,Enter,Wait,Wait,Enter"),.5f);}
    else
    {
     const auto Selected=M->Flow->Saves.ContinueSave();
     const FString Expected=S47J::Str(Base,TEXT("save"));
     if(!Check(Selected&&Selected->Id==Expected&&Selected->SavedAt==S47J::Num(Base,TEXT("saveAt")),FString::Printf(TEXT("title Continue selects checkpoint id=%s savedAt=%.0f; expected id=%s savedAt=%.0f"),Selected?*Selected->Id:TEXT("<none>"),Selected?Selected->SavedAt:-1.,*Expected,S47J::Num(Base,TEXT("saveAt")))))return false;
     S47Keys::TestPresses(TEXT("Enter"),.5f);
    }
    Go(1);return true;
   }
   if(Stage==1)
   {
    if(M->Mode!=US47Main::EMode::Play||M->Flow->bStarting||M->Flow->bRestoringNow)return true;
    if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active(),TEXT("normal case1, never developer case0")))return false;
    auto& F=*M->Freeform;
    if(Phase==1)
    {
     if(!Check(F.InTutorial()&&F.CurrentJob()&&F.Field&&F.Field->ParcelStage()==0,TEXT("untouched tutorial and no parcel")))return false;
     Job=F.CurrentJob()->Id;S47J::Set(Base,TEXT("job"),Job);S47J::Set(Base,TEXT("cash"),F.Budget());S47J::Set(Base,TEXT("fuse"),F.InventoryCount(TEXT("fuse")));S47J::Set(Base,TEXT("relay"),F.InventoryCount(TEXT("relay")));S47J::Set(Base,TEXT("delivered"),F.Field->DeliveredParcels());S47J::Set(Base,TEXT("cargo"),F.Field->CargoDeliveries());
     if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(10);
    }
    else if(Phase>=4)
    {
     Job=S47J::Str(Base,TEXT("job"));Photo=S47J::Str(Base,TEXT("photo"));
     if(!Retained()||!Check(F.Director&&Shot()->IsDeveloped()&&!F.InTutorial()&&F.Budget()==S47J::Num(Base,TEXT("cash"))+(Phase>=5?5:-15)&&F.InventoryCount(TEXT("fuse"))==S47J::Int(Base,TEXT("fuse"))+2&&F.InventoryCount(TEXT("relay"))==S47J::Int(Base,TEXT("relay"))+1&&F.Field->ParcelStage()==0&&F.Field->DeliveredParcels()==S47J::Int(Base,TEXT("delivered"))+1&&F.Field->CargoDeliveries()==S47J::Int(Base,TEXT("cargo"))+1,TEXT("later restart preserves completed delivery and phase-specific exact economy")))return false;
     if(Phase>=5&&!EventRetained(true))return false;
     if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(Phase>=6?(Phase==6?600:700):(Phase==4?400:430));
    }
    else {Job=S47J::Str(Base,TEXT("job"));Photo=S47J::Str(Base,TEXT("photo"));if(!Retained()||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))+(Phase==2?20:-15)&&F.InventoryCount(TEXT("fuse"))==S47J::Int(Base,TEXT("fuse"))&&F.InventoryCount(TEXT("relay"))==S47J::Int(Base,TEXT("relay")),TEXT("restart preserves exact earned/spent cash and not-yet-delivered stock")))return false;if(Phase==2){if(!Check(Shot()->LabStage==1,TEXT("normal Continue retains processed-film stage"))||!Place(TEXT("wetBench")))return false;Go(40);}else {if(!Check(Shot()->IsDeveloped(),TEXT("second process restart retains the actually developed original")))return false;Go(60);}}
    return true;
   }
   if(M->Flow->bStarting||M->Flow->bRestoringNow||!M->Freeform||!M->Freeform->Active())return true;
   auto& F=*M->Freeform;auto& W=*F.Field;
   if(Stage>=601&&Stage<=608)
   {
    const auto* J=F.CurrentJob();if(!J||J->Id!=CaptureJob)return Finish(false,TEXT("same selected capture job lost"));
   if(Stage==601){if(F.StartAcquisition()){Go(602);}return true;}
   if(Stage==602)
   {
    if(!J->CaptureOriginal.Failure.IsEmpty())return Finish(false,J->CaptureOriginal.Failure);
    if(J->Elapsed>=6&&!ChangedDials){const auto* P=F.ConsoleProfile();M->Story->Rx.Gain=P->Gain[1];M->Story->Rx.Bandwidth=P->Bw[1];ChangedDials=true;}
    if(J->Stage!=FS47FreeformSession::EJobStage::Chart||F.PrinterProgress()<1)return true;
    if(!Check(J->CaptureOriginal.Playable()&&ChangedDials&&J->CaptureOriginal.Frames[0].Gain!=J->CaptureOriginal.Frames.Last().Gain,TEXT("actual full-rate capture retains real mid-recording dial change")))return false;
    const auto State=F.CaptureSoundState();if(S47J::Num(State,TEXT("liveRendered"))<18*M->Audio->Ctx->SampleRate)return true;
    Evidence.Add(MakeShared<FJsonValueObject>(State));Original=S47J::Write(J->CaptureOriginal.Save());Ticks=J->TraceTicks;
    M->UI->Close(true);const auto* It=M->Inter->Get(TEXT("printer"));const FVector T=It&&It->Object?It->Object.WorldBox().GetCenter():S47Three::ToThree(M->Web->PropLocation(TEXT("printer")));
    M->Player->Place(T.X,T.Z+1,0);const FVector D=T-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());Go(603);return true;
   }
   if(Stage==603)
   {
    if(Age-At<.4)return true;
    if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("printer")&&bool(M->Player->OnUse),TEXT("actual printer selectable by physical eye ray")))return false;
    M->Player->OnUse(TEXT("printer"));M->UI->Close(true);J=F.CurrentJob();
    if(!Check(J->Stage==FS47FreeformSession::EJobStage::Recorded&&!J->OriginalPage.IsEmpty(),TEXT("actual printed original retained")))return false;Document=J->OriginalPage;
    const auto* P=F.ConsoleProfile();M->Story->Rx.Gain=P->Gain[0];M->Story->Rx.Bandwidth=P->Bw[0];M->Player->Place(0,-2.15,0);F.OpenReceiver();
    if(!Check(F.StartControl(),TEXT("production eight-second internal reference begins with own settings")))return false;Go(604);return true;
   }
   if(Stage==604)
   {
    if(!J->CaptureControl.Failure.IsEmpty())return Finish(false,J->CaptureControl.Failure);
    if(J->ControlAt<8||S47J::Num(F.CaptureSoundState(),TEXT("liveRendered"))<8*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->CaptureControl.Playable()&&J->CaptureControl.Control&&J->CaptureControl.Ticks.IsEmpty()&&S47J::Write(J->CaptureOriginal.Save())==Original&&J->TraceTicks==Ticks&&J->OriginalPage==Document,TEXT("actual reference fully rendered independently; original and evidence ticks unchanged")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));Control=S47J::Write(J->CaptureControl.Save());
    M->UI->Close(true);if(!Check(F.AudioDesk&&F.AudioDesk->Ready(),TEXT("physical audio comparator exists")))return false;const auto T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.2,0);Go(605);return true;
   }
   if(Stage==605)
   {
    if(Age-At<.3)return true;if(!Check(F.AudioDesk->ListeningPosition(),TEXT("actual player stands within powered reel-deck listening area")))return false;F.OpenAudio();F.AudioAction(TEXT("original"));F.SeekAudio(7.25);
    if(!Check(F.AudioAction(TEXT("play"))&&F.AudioPreparing(),TEXT("explicit PLAY requests tape preparation")))return false;
    if(!Check(F.AudioDesk->Use(TEXT("ff:audio:play"))&&!F.AudioPreparing()&&!F.AudioPlaying()&&F.AudioPosition()==7.25&&J->OriginalCovered==0,TEXT("physical PLAY STOP callback cancels preparation without moving cursor or coverage")))return false;
    if(!Check(F.AudioAction(TEXT("play")),TEXT("actual retained transport seeks and plays original")))return false;Go(606);return true;
   }
   if(Stage==606)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==7.25&&J->OriginalCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<8.25)return Finish(false,TEXT("seek playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<8.25)return true;F.AudioAction(TEXT("pause"));
    if(!Check(J->OriginalCovered==0,TEXT("seeking across unheard beginning does not grant listening coverage")))return false;
    F.AudioAction(TEXT("control"));if(!Check(F.AudioAction(TEXT("play")),TEXT("actual independent control playback starts")))return false;Go(607);return true;
   }
   if(Stage==607)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==0&&J->ControlCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<8)return Finish(false,TEXT("control playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<8||S47J::Num(F.CaptureSoundState(),TEXT("tapeRendered"))<8*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->ControlCovered>=8,TEXT("complete actual control listening retains existing coverage rule")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));F.AudioAction(TEXT("original"));if(!Check(F.AudioAction(TEXT("play")),TEXT("original playback restarts at zero after prior seek")))return false;Go(608);return true;
   }
   if(Stage==608)
   {
    if(F.AudioPreparing()){if(!(F.AudioPosition()==0&&J->OriginalCovered==0)){Check(false,TEXT("tape readiness advanced cursor or unheard coverage"));return false;}LoadingSeen=Stage;return true;}
    if(LoadingChecked!=Stage){if(!Check(LoadingSeen==Stage,TEXT("observed tape readiness held cursor and unheard coverage throughout preparation")))return false;LoadingChecked=Stage;}
    if(!F.AudioPlaying()&&!F.AudioPreparing()&&F.AudioPosition()<18)return Finish(false,TEXT("original playback stopped early: ")+F.FeedbackText()+TEXT(" / ")+F.AudioDisplay());
    if(F.AudioPosition()<18||S47J::Num(F.CaptureSoundState(),TEXT("tapeRendered"))<18*M->Audio->Ctx->SampleRate)return true;
    if(!Check(J->OriginalCovered>=18&&J->ControlCovered>=8&&J->TraceTicks==Ticks&&J->OriginalPage==Document,TEXT("full actual playback keeps all evidence positions and original bytes")))return false;
    Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));
    S47J::Set(Base,TEXT("captureOriginal"),J->CaptureOriginal.Save());S47J::Set(Base,TEXT("captureControl"),J->CaptureControl.Save());
    S47J::Set(Base,TEXT("captureDocumentHash"),Hash(J->OriginalPage));S47J::Set(Base,TEXT("captureStamp"),J->OriginalStamp);S47J::Set(Base,TEXT("captureTicks"),J->TraceTicks);
    S47J::Set(Base,TEXT("captureOriginalCovered"),J->OriginalCovered);S47J::Set(Base,TEXT("captureControlCovered"),J->ControlCovered);
    if(!CaptureRetained()||!EventRetained(true)||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))+5&&!F.AudioPlaying(),TEXT("capture/listening grants no payment and transport has stopped before normal save")))return false;
    SaveMenu();return true;
   }
    return !Done;
   }
   switch(Stage)
   {
    case 10:if(!Use(TEXT("opsTerminal")))return false;Go(11);break;
    case 11:if(!Click(TEXT("DAILY WORK")))return false;Go(12);break;
    case 12:if(!Click(TEXT("Accept this job and start the shift"))||!Place(TEXT("crtRight")))return false;Go(13);break;
    case 13:if(!Use(TEXT("crtRight")))return false;Go(14);break;
    case 14:if(!Click(TEXT("POINT ARRAY"))||!Place(TEXT("crtCenter")))return false;Go(15);break;
    case 15:if(!Use(TEXT("crtCenter")))return false;Go(16);break;
    case 16:if(Tune())Go(17);break;
    case 17:if(!Enabled(TEXT("START RECORDING")))return true;if(!Click(TEXT("START RECORDING")))return false;M->UI->Close(true);Go(18);break;
    case 18:if(!F.CurrentJob()||F.CurrentJob()->Stage!=FS47FreeformSession::EJobStage::Chart||F.PrinterProgress()<1)return true;if(!Place(TEXT("printer")))return false;Go(19);break;
    case 19:if(!Use(TEXT("printer")))return false;Go(20);break;
    case 20:
     if(!Check(F.CurrentJob()&&F.CurrentJob()->Stage==FS47FreeformSession::EJobStage::Recorded&&!F.CurrentJob()->OriginalPage.IsEmpty(),TEXT("real recording and printer produce original")))return false;
     S47J::Set(Base,TEXT("originalHash"),Hash(F.CurrentJob()->OriginalPage));S47J::Set(Base,TEXT("stamp"),F.CurrentJob()->OriginalStamp);if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(21);break;
    case 21:if(!Use(TEXT("opsTerminal")))return false;Go(22);break;
    case 22:if(!Click(TEXT("DAILY WORK")))return false;Go(23);break;
    case 23:if(!Click(TEXT("Register routine result / $20")))return false;Go(24);break;
    case 24:if(!Check(!F.InTutorial()&&F.Budget()==S47J::Num(Base,TEXT("cash"))+20,TEXT("earned routine payment exactly once"))||!Place(TEXT("fieldCamera"),FVector(-1.3,0,0)))return false;Go(25);break;
    case 25:if(!Use(TEXT("fieldCamera")))return false;if(M->Night->Album->Left()<=0){if(!Place(TEXT("ff:film"),FVector(-1.3,0,0)))return false;Go(250);}else Go(26);break;
    case 250:if(!Use(TEXT("ff:film")))return false;Go(26);break;
    case 26:
     if(!Check(M->FieldCamera->bHave&&M->Night->Album->Left()>0,TEXT("normal acquired camera has real film; no stock injected")))return false;
     M->UI->Close(true);M->Player->Place(0,1.5,0);Aim(FVector(0,1,-4));Go(27);break;
    case 27:
    {
     M->FieldCamera->Raise(true);M->FieldCamera->Sync();
     if(!Check(M->FieldCamera->PhotoLoc.Equals(M->Player->EyeUE(),.01)&&M->FieldCamera->PhotoRot.Equals(M->Player->GetActorRotation(),.01),TEXT("photo capture pose matches actual viewport walker after Sync")))return false;
     const FVector Eye=S47Three::ToThree(M->FieldCamera->PhotoLoc),Forward=S47Three::ToThree(M->FieldCamera->PhotoRot.Vector()).GetSafeNormal();
     const auto P=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
     // Album only stamps a valid Pose when its real FS47Snap.bRendered succeeds.
     // Expose returns FS47AlbumShot, which has no bRendered member of its own.
     if(!Check(!P.bUnavailable&&P.Url.StartsWith(TEXT("data:image/"))&&P.Pose.IsSet()&&P.Pose->Valid()&&P.Pose->Eye.Equals(Eye,.0001)&&P.Pose->Forward.Equals(Forward,.0001)&&P.Pose->Scene==M->Web->Area(),TEXT("render succeeded and photograph retains actual synced viewport pose; no fallback image")))return false;
     Photo=P.Id;S47J::Set(Base,TEXT("photo"),Photo);S47J::Set(Base,TEXT("photoHash"),Hash(P.Url));S47J::Set(Base,TEXT("film"),M->Night->Album->Left());if(!Place(TEXT("wetBench")))return false;Go(28);break;
    }
    case 28:if(M->Web->IsDoorOpen(TEXT("labDoor")))return Finish(false,TEXT("lab door open: close through ordinary door input; harness will not force it"));if(!Use(TEXT("wetBench")))return false;Go(29);break;
    case 29:if(!Click(TEXT("LOAD SEALED FILM INTO TANK")))return false;Go(30);break;
    case 30:if(!Check(Shot()&&Shot()->LabStage==1&&F.Budget()==S47J::Num(Base,TEXT("cash"))+20,TEXT("actual first lab action retained; no photo payment")))return false;SaveMenu();break;
    case 40:if(!Use(TEXT("wetBench")))return false;Go(41);break;
    case 41:if(!Click(TEXT("TRANSFER CONTACT PRINT TO FIXER")))return false;Go(42);break;
    case 42:if(!Check(Shot()&&Shot()->LabStage==2,TEXT("actual fixer action completed"))||!Click(TEXT("COLLECT AND EXAMINE PRINT")))return false;Go(43);break;
    case 43:if(!Check(Shot()&&Shot()->IsDeveloped()&&F.Budget()==S47J::Num(Base,TEXT("cash"))+20,TEXT("same developed ordinary photo grants no payment")))return false;M->UI->Close(true);M->World->DriveToDiner(nullptr);M->World->Autopilot=12.;TravelAt=Age;Go(44);break;
    case 44:if(!Travel(TEXT("diner")))return true;if(!AS47Props::IsDoorOpen(TEXT("gasDoor"))){if(!Place(TEXT("town:gasDoor"),FVector(0,0,-1)))return false;Go(440);}else Go(441);break;
    case 440:if(!Use(TEXT("town:gasDoor")))return false;Go(441);break;
    case 441:if(!Check(AS47Props::IsDoorOpen(TEXT("gasDoor")),TEXT("actual shop door opened before interior placement")))return false;M->Player->Place(7981.8,2068.8,0);Aim(W.InteractionPoint(TEXT("ff:shop")));Go(45);break;
    case 45:if(!Check(W.ShopOpen(),TEXT("shop actually open at natural clock"))||!Use(TEXT("ff:shop")))return false;Go(46);break;
    case 46:if(!Click(TEXT("control fuse / $5")))return false;Go(47);break;
    case 47:if(!Click(TEXT("drive relay / $25")))return false;Go(48);break;
    case 48:if(!Click(TEXT("control fuse / $5")))return false;Go(49);break;
    case 49:if(!Check(W.ParcelStage()==1&&W.ParcelContents().Num()==2&&W.ParcelContents().FindRef(TEXT("fuse"))==2&&W.ParcelContents().FindRef(TEXT("relay"))==1&&F.Budget()==S47J::Num(Base,TEXT("cash"))-15&&F.InventoryCount(TEXT("fuse"))==S47J::Int(Base,TEXT("fuse"))&&F.InventoryCount(TEXT("relay"))==S47J::Int(Base,TEXT("relay")),TEXT("one paid untouched counter parcel; exact charge, no premature stock")))return false;SaveMenu();break;
    case 60:if(!Check(W.ParcelStage()==1&&W.ParcelContents().FindRef(TEXT("fuse"))==2&&W.ParcelContents().FindRef(TEXT("relay"))==1&&F.Budget()==S47J::Num(Base,TEXT("cash"))-15,TEXT("normal Continue retains paid counter basket")))return false;M->UI->Close(true);M->Player->Place(7981.8,2068.8,0);Aim(W.InteractionPoint(TEXT("ff:parcel")));Go(61);break;
    case 61:if(!Use(TEXT("ff:parcel"))||!Place(TEXT("ff:cargo"),FVector(1.4,0,0)))return false;Go(62);break;
    case 62:
    {
     if(!Use(TEXT("ff:cargo"))||!Check(W.ParcelStage()==3,TEXT("actual cargo action loads paid package")))return false;
     const FVector Truck=M->World->ServicePosition();const double H=M->World->ServiceHeading();
     const FVector Front=Truck+FVector(-FMath::Sin(H),.9,-FMath::Cos(H)),Stand=Front+FVector(1.8*FMath::Cos(H),0,-1.8*FMath::Sin(H));
     M->Player->Place(Stand.X,Stand.Z,0);Aim(Front);Go(620);break;
    }
    case 620:if(!Check(M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())==TEXT("dinerTruck"),TEXT("aiming at cab retains ordinary diner boarding while cargo is loaded")))return false;M->World->DriveBack();M->World->Autopilot=12.;TravelAt=Age;Go(63);break;
    case 63:if(!Travel(TEXT("saro")))return true;if(!Place(TEXT("ff:cargo"),FVector(1.4,0,0)))return false;Go(64);break;
    case 64:if(!Use(TEXT("ff:parcel"))||!Check(W.ParcelStage()==2,TEXT("actual loaded parcel pickup unloads the same cargo"))||!Place(TEXT("ff:delivery")))return false;Go(65);break;
    case 65:if(!Use(TEXT("ff:delivery")))return false;Go(66);break;
    case 66:
     if(!Check(W.ParcelStage()==0&&W.ParcelContents().IsEmpty()&&F.InventoryCount(TEXT("fuse"))==S47J::Int(Base,TEXT("fuse"))+2&&F.InventoryCount(TEXT("relay"))==S47J::Int(Base,TEXT("relay"))+1&&W.DeliveredParcels()==S47J::Int(Base,TEXT("delivered"))+1&&W.CargoDeliveries()==S47J::Int(Base,TEXT("cargo"))+1&&F.Budget()==S47J::Num(Base,TEXT("cash"))-15,TEXT("actual transport and one delivery preserve exact economy"))||!Retained())return false;SaveMenu();break;
    case 399:
    {
     if(F.Director->EventId()==TEXT("dusk_orb")){M->UI->Close(true);Go(405);break;}
     const auto D=F.Director->Save();
     if(F.Director->Busy()||S47J::Num(D,TEXT("activeTime"))<S47J::Num(D,TEXT("restUntil")))
     {if(Age-At>600)return Finish(false,TEXT("natural encounter/cooldown did not finish during bounded active wait"));return true;}
     if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(400);break;
    }
    case 400:
     if(!Check(!F.PowerFault()||W.GeneratorRunning(),TEXT("station power available; harness never repairs or supplies power"))||!Use(TEXT("opsTerminal")))return false;Go(401);break;
    case 401:
    {
     if(F.Director->EventId()==TEXT("dusk_orb")){M->UI->Close(true);Go(405);break;}
     const auto D=F.Director->Save();const auto Orb=S47J::Obj(D,TEXT("duskOrb"));
     if(F.Director->Busy()||S47J::Num(D,TEXT("activeTime"))<S47J::Num(D,TEXT("restUntil")))
     {M->UI->Close(true);Go(399);break;}
     if(!Check(!S47J::Bool(Orb,TEXT("photographed")),TEXT("unphotographed natural invitation; any prior cooldown elapsed through active time")))return false;
     const double Dusk=FMath::FloorToDouble(F.Clock()/86400.)*86400.+19*3600.;
     const bool NextDay=S47J::Num(Orb,TEXT("dueClock"))>Dusk||F.Clock()>=Dusk-60;
     if(!Check(++Waits<=3,TEXT("bounded ordinary wait sequence"))||!Click(NextDay?TEXT("REST UNTIL MORNING"):TEXT("Wait until dusk / review time and equipment first")))return false;
     Go(NextDay?402:404);break;
    }
    case 402:if(!Click(TEXT("Wait until the stated time")))return false;Go(403);break;
    case 403:if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(400);break;
    case 404:if(!Click(TEXT("Wait until the stated time")))return false;Go(405);break;
    case 405:
    {
     if(F.Director->EventId()!=TEXT("dusk_orb"))
     {if(F.Director->Busy()||Age-At>65)return Finish(false,TEXT("natural dusk invitation blocked or missed; no Force/retry state injection"));return true;}
     if(!Check(!F.PowerFault()||W.GeneratorRunning(),TEXT("power still available after real wait fuel consequences")))return false;
     M->UI->Close(true);const FVector P=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());M->Player->Place(P.X,P.Z+6,0);Aim(P);Go(406);break;
    }
    case 406:
    {
     if(Age-At<2.4)return true;
     if(!Check(F.Director->EventId()==TEXT("dusk_orb")&&M->FieldCamera->bHave&&M->Night->Album->Left()>0,TEXT("natural live orb and owned camera with film")))return false;
     M->FieldCamera->Raise(true);M->FieldCamera->Sync();
     const FVector Eye=S47Three::ToThree(M->FieldCamera->PhotoLoc),Forward=S47Three::ToThree(M->FieldCamera->PhotoRot.Vector()).GetSafeNormal();
     if(!Check(M->FieldCamera->PhotoLoc.Equals(M->Player->EyeUE(),.01)&&M->FieldCamera->PhotoRot.Equals(M->Player->GetActorRotation(),.01),TEXT("event shutter uses actual synced viewport")))return false;
     const auto P=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
     if(!Check(P.Id!=Photo&&!P.bUnavailable&&P.Url.StartsWith(TEXT("data:image/"))&&P.Pose.IsSet()&&P.Pose->Valid()&&P.Pose->Scene==M->Web->Area()&&P.Pose->Eye.Equals(Eye,.0001)&&P.Pose->Forward.Equals(Forward,.0001),TEXT("new event original has rendered image and authentic shutter pose")))return false;
     const auto* C=F.Director->PhotoClaims().FindByPredicate([&](const auto& X){return X.PhotoId==P.Id&&X.Id.StartsWith(TEXT("dusk_orb."));});
     if(!Check(C&&!C->bPaid&&C->Reward==20,TEXT("production image qualification creates unpaid dusk incident")))return false;
     S47J::Set(Base,TEXT("eventPhoto"),P.Id);S47J::Set(Base,TEXT("eventHash"),Hash(P.Url));S47J::Set(Base,TEXT("eventClock"),P.Clock);S47J::Set(Base,TEXT("eventClaim"),C->Id);
     if(!Retained()||!Place(TEXT("wetBench")))return false;Go(410);break;
    }
    case 410:if(!Check(!M->Web->IsDoorOpen(TEXT("labDoor")),TEXT("lab door must already be closed; no forced door state"))||!Use(TEXT("wetBench")))return false;Go(411);break;
    case 411:if(!Click(TEXT("LOAD SEALED FILM INTO TANK")))return false;Go(412);break;
    case 412:if(!Check(EventShot()&&EventShot()->LabStage==1,TEXT("event sealed film processed"))||!Click(TEXT("TRANSFER CONTACT PRINT TO FIXER")))return false;Go(413);break;
    case 413:if(!Check(EventShot()&&EventShot()->LabStage==2,TEXT("event print fixed"))||!Click(TEXT("COLLECT AND EXAMINE PRINT")))return false;Go(414);break;
    case 414:
     if(!EventRetained(false)||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))-15,TEXT("development alone grants no credit")))return false;
     FScreenshotRequest::RequestScreenshot(Root/TEXT("phase-4-developed-event.png"),true,false);Go(415);break;
    case 415:if(Age-At<1)return true;if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(420);break;
    case 420:if(!Use(TEXT("opsTerminal")))return false;Go(421);break;
    case 421:if(!Click(TEXT("SUPPLIES")))return false;Go(422);break;
    case 422:if(!Click(TEXT("Register photograph / $20 documentation credit")))return false;Go(423);break;
    case 423:
     if(!EventRetained(true)||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))+5&&!VisibleText(TEXT("Register photograph / $20 documentation credit")),TEXT("native registration grants exactly $20 and removes repeat-payment action")))return false;
     if(!Place(TEXT("opsTerminal"),FVector(1,0,0)))return false;Go(430);break;
    case 430:if(!Use(TEXT("opsTerminal")))return false;Go(431);break;
    case 431:if(!Click(TEXT("SUPPLIES")))return false;Go(432);break;
    case 432:
     if(!EventRetained(true)||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))+5&&VisibleText(TEXT("ORIGINAL REGISTERED / Documentation credit already received."))&&!VisibleText(TEXT("Register photograph / $20 documentation credit")),TEXT("reopened native OPS retains paid original and offers no second payment, including after process restart")))return false;
     SaveMenu();break;
    case 600:
    {
     // New games start worn and audio capture needs full rate: the whole array is a declared fixture here.
     if(!Check(F.Field->ArrayWear().bActive&&F.AvailableDishCount()==12,TEXT("new game still on its worn twelve of twenty-seven")))return false;
     F.Field->RestoreWholeArrayFixture();
     if(!CaptureHealth())return false;
     const int32 Index=F.Jobs().IndexOfByPredicate([](const auto& J){return J.Source==1&&J.Day==1&&J.Slot==1&&J.CaseTest<0;});
     if(!Check(F.Jobs().IsValidIndex(Index)&&(F.Jobs()[Index].Stage==FS47FreeformSession::EJobStage::New||F.Jobs()[Index].Stage==FS47FreeformSession::EJobStage::Pointed)&&F.Jobs()[Index].Elapsed==0&&!F.Jobs()[Index].CaptureOriginal.Enabled(),TEXT("existing untouched day1 pulsar; no job creation, reset or injected capture")))return false;
     CaptureJob=F.Jobs()[Index].Id;S47J::Set(Base,TEXT("captureJob"),CaptureJob);const int32 Count=F.Jobs().Num();
     M->UI->Close(true);if(!Check(F.SelectJob(Index),TEXT("explicit selection of existing first-day pulsar card")))return false;
     F.SetAnomaly(true);if(!Check(F.Jobs().Num()==Count&&F.CurrentJob()&&F.CurrentJob()->Id==CaptureJob,TEXT("explicit discrepancy fixture changes no job identity/count or acquired data")))return false;
     const auto* P=F.ConsoleProfile();if(!Check(P!=nullptr&&F.CurrentJob()->bGenerated&&F.CurrentJob()->Card.Seconds==18,TEXT("actual existing18s pulsar profile")))return false;
     M->Story->Rx.Frequency=P->F;M->Story->Rx.Gain=P->Gain[0];M->Story->Rx.Bandwidth=P->Bw[0];M->Player->Place(0,-2.15,0);
     if(!Check(F.PointSelected(),TEXT("production pointing command accepted")))return false;F.OpenReceiver();Go(601);break;
    }
    case 700:
    {
     CaptureJob=S47J::Str(Base,TEXT("captureJob"));
     if(!CaptureHealth()||!CaptureRetained()||!Check(F.CurrentJob()&&F.CurrentJob()->Id==CaptureJob,TEXT("ordinary title Continue restores selected captured pulsar")))return false;
     if(!Check(F.AudioDesk&&F.AudioDesk->Ready(),TEXT("restored physical reel deck exists")))return false;
     // Different current dial settings cannot rewrite either retained recipe.
     const auto* P=F.ConsoleProfile();if(!Check(P!=nullptr,TEXT("restored real card profile")))return false;
     M->Story->Rx.Gain=P->Gain[1];M->Story->Rx.Bandwidth=P->Bw[1];
     M->UI->Close(true);const auto T=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));M->Player->Place(T.X,T.Z-1.2,0);Go(701);break;
    }
    case 701:
     if(!Check(F.AudioDesk->ListeningPosition(),TEXT("actual deck listening area after process restart")))return false;
     F.OpenAudio();F.AudioAction(TEXT("original"));if(!Check(F.AudioAction(TEXT("play")),TEXT("normal retained original transport starts after process restart")))return false;Go(702);break;
    case 702:
    case 703:
    {
     if(F.AudioPreparing())return true;
     const double Duration=Stage==702?18.:8.;
     if(!F.AudioPlaying()&&F.AudioPosition()<Duration)return Finish(false,TEXT("restored captured transport stopped before full original/control rendering: ")+F.FeedbackText());
     if(F.AudioPosition()<Duration||S47J::Num(F.CaptureSoundState(),TEXT("tapeRendered"))<Duration*M->Audio->Ctx->SampleRate)return true;
     Evidence.Add(MakeShared<FJsonValueObject>(F.CaptureSoundState()));
     if(!CaptureRetained())return false;
     if(Stage==702){F.AudioAction(TEXT("control"));if(!Check(F.AudioAction(TEXT("play")),TEXT("normal retained internal-control transport starts after process restart")))return false;Go(703);break;}
     if(!EventRetained(true)||!Check(F.Budget()==S47J::Num(Base,TEXT("cash"))+5&&F.InventoryCount(TEXT("fuse"))==S47J::Int(Base,TEXT("fuse"))+2&&F.InventoryCount(TEXT("relay"))==S47J::Int(Base,TEXT("relay"))+1,TEXT("actual restarted playback preserves older photo/economy and delivered stock")))return false;
     S47J::Set(Base,TEXT("completedPhase"),7);if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Base,true),*(Root/TEXT("journey.json"))),TEXT("final verification receipt written without an extra manual save")))return false;
     return Finish(true,TEXT("ordinary Save Case, separate process and exact title Continue preserve actual v3 original/control; both render through real transport again"));
    }
    case 890:
     if(Age-At<2.5)return true;
     S47Keys::TestPresses(Phase==4?TEXT("Left,Left,Enter"):(Phase==6?TEXT("Left,Left,Right,Right,Enter"):TEXT("Left,Left,Right,Enter")),.5f);Go(891);break;
    case 891:
     if(Age-At<3||(!VisibleText(FString::Printf(TEXT("Overwrite save %d?"),SaveSlot()+1))&&Age-At<6))return true;
     if(!Check(VisibleText(FString::Printf(TEXT("Overwrite save %d?"),SaveSlot()+1)),TEXT("normal dialog names explicitly selected occupied slot")))return false;S47Keys::TestPresses(TEXT("Enter"),.5f);Go(900);break;
    case 900:
    {
     if(Age-At<4)return true;TOptional<FS47SaveMeta> Saved;for(const auto& S:M->Flow->Saves.List(1))if(S.Kind==TEXT("manual")&&S.SavedAt>=SaveAt&&S.Slot==SaveSlot())Saved=S;
     if(!Saved){if(Age-At>15)return Finish(false,TEXT("manual Save Case UI did not create expected fresh slot"));return true;}
     const auto Disk=M->Flow->Saves.Load(Saved->Id);if(!Check(Disk&&S47J::Str(Disk,TEXT("sessionMode"))==TEXT("freeform")&&S47J::Num(S47J::Obj(Disk,TEXT("freeform")),TEXT("cash"))==F.Budget()&&M->Flow->Saves.Active()==1,TEXT("ordinary manual save read from real store with correct mode, active case and cash")))return false;
     for(const auto& Pair:PreservedSaves)
     {
      const auto Other=M->Flow->Saves.Load(Pair.Key);
      if(!Check(Other.IsValid(),TEXT("non-target manual save readable after overwrite: ")+Pair.Key)||!Check(Hash(S47J::Write(Other))==Pair.Value,TEXT("non-target manual save unchanged: ")+Pair.Key))return false;
     }
     S47J::Set(Base,TEXT("completedPhase"),Phase);S47J::Set(Base,TEXT("save"),Saved->Id);S47J::Set(Base,TEXT("saveAt"),Saved->SavedAt);FFileHelper::SaveStringToFile(S47J::Write(Base,true),*(Root/TEXT("journey.json")));
     FScreenshotRequest::RequestScreenshot(Root/FString::Printf(TEXT("phase-%d-save.png"),Phase),true,false);Go(901);break;
    }
    case 901:
    {
     TArray<uint8> Png;const FString File=Root/FString::Printf(TEXT("phase-%d-save.png"),Phase);
     const uint8 End[]={0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};
     const bool Written=FFileHelper::LoadFileToArray(Png,*File)&&Png.Num()>1024&&Png[0]==137&&FMemory::Memcmp(Png.GetData()+Png.Num()-8,End,8)==0;
     if(!Written){if(Age-At>15)return Finish(false,TEXT("save screenshot did not finish writing"));return true;}
     return Finish(true,TEXT("normal manual checkpoint reached; restart next phase in a new process"));
    }
   }
   return !Done;
  }
 };
}
void S47NormalJourney::Start(US47Main* M)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47NormalJourney")))return;static bool Started=false;if(Started)return;Started=true;
 auto R=MakeShared<FRun>();R->Main=M;FParse::Value(FCommandLine::Get(),TEXT("S47JourneyPhase="),R->Phase);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),R->User);
 const FString Dir=FPaths::ConvertRelativePathToFull(R->User);const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
 if(R->User.Contains(TEXT(".."))||!R->User.StartsWith(TEXT("/tmp/S47-normal-journey-"))||!FPaths::IsUnderDirectory(Saved,Dir)||R->Phase<1||R->Phase>7||!FParse::Param(FCommandLine::Get(),TEXT("S47Title"))||!FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest"))||FParse::Param(FCommandLine::Get(),TEXT("S47Freeform"))||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTest")))
 {R->Finish(false,TEXT("requires isolated normal-journey UserDir, title+smoke, phase1..7 and no developer Freeform flags"));return;}
 if(R->Phase>=6&&(!FParse::Param(FCommandLine::Get(),TEXT("S47JourneyCapture"))||FParse::Param(FCommandLine::Get(),TEXT("NoSound"))||FParse::Param(FCommandLine::Get(),TEXT("S47CaptureSoundTest")))){R->Finish(false,TEXT("capture phases require explicit S47JourneyCapture, real audio, and no parallel capture driver"));return;}
 R->Root=Saved/TEXT("NormalJourney");IFileManager::Get().MakeDirectory(*R->Root,true);
 if(R->Phase>1){FString Text;if(!FFileHelper::LoadFileToString(Text,*(R->Root/TEXT("journey.json")))||!(R->Base=S47J::Read(Text))||S47J::Int(R->Base,TEXT("completedPhase"))!=R->Phase-1){R->Finish(false,TEXT("previous process checkpoint receipt missing"));return;}}
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float Dt){return R->Tick(Dt);}));
}
