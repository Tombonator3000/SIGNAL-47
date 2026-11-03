#include "S47FreeformEvidenceTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformAudioDesk.h"
#include "S47Observing.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
 using EJob=FS47FreeformSession::EJobStage;
 struct FOriginal {FString Id,Page,Stamp;};
 struct FRun
 {
  bool Started=false,Done=false,Walking=false,HaveFoot=false,Quarantined=false;
  int32 Stage=0,Job=0,Cursor=0,Cross=0,Knob=0,Checks=0,InitialCash=0,InitialDebt=0,Restores=0;
  double Began=0,At=0,Delay=.25,ProgressAt=0,WalkMetres=0,SampleAt=0,PhotoAt=0,BaseAz=0,BaseEl=0;
  FString Root,Ids[2],PendingPhoto,PickFailure;
  double PickFailureAt=0;
  FVector LastFoot=FVector::ZeroVector,ProgressFoot=FVector::ZeroVector,SavedFoot=FVector::ZeroVector;
  TArray<FVector2D> Path;
  TWeakPtr<SSlider> LockedSlider;
  TArray<FOriginal> Originals;
  TArray<FString> Photos;
  TArray<TSharedPtr<FJsonValue>> Evidence,Route;
 } S;
 void Go(int32 Stage,double Delay=.25){S.Stage=Stage;S.At=FPlatformTime::Seconds();S.Delay=Delay;}
 S47J::FObj Vec(const FVector& V){auto O=S47J::New();S47J::Set(O,TEXT("x"),V.X);S47J::Set(O,TEXT("y"),V.Y);S47J::Set(O,TEXT("z"),V.Z);return O;}
 void Finish(US47Main* M,bool Good,FString Why)
 {
  if(S.Done)return;S.Done=true;if(M&&M->Player)M->Player->TestMove.Reset();
  if(Good)for(const FString& Name:S.Photos)
  {TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*(S.Root/(Name+TEXT(".png"))))||B.Num()<1024||B[0]!=137||B[1]!=80||B[2]!=78||B[3]!=71){Good=false;Why=TEXT("required native PNG missing: ")+Name;}}
  auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("stage"),S.Stage);S47J::Set(O,TEXT("checks"),S.Checks);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-S.Began);S47J::Set(O,TEXT("walkMetres"),S.WalkMetres);S47J::Set(O,TEXT("ordinaryContinues"),S.Restores);S47J::Set(O,TEXT("crossPointsObserved"),S.Cross);
  S47J::Set(O,TEXT("fixtures"),TEXT("Fresh isolated Freeform case0. One initial OPS approach placement. Initial Director nextAt is delayed beyond this bounded scenario, retaining its seed/history; normal reference is measured before the one named extra_pulse Force fixture. No job completion, chart, audio coverage, marks, encoders, inventory or reward is injected. Final isolated case1/save-store metadata is established solely to exercise the real production save/title/Continue path."));
  S47J::Set(O,TEXT("method"),TEXT("Actual Walker TestMove/Resolve between stations; actual camera eye-ray Pick and Walker OnUse for apparatus, printer, doors, tape keys and binder; unique live native Slate button actions and slider navigation callbacks; actual five-point array cross and pulsar/internal-reference acquisition and listening; physical filing; ordinary WriteSave/fresh disk reader/title Continue and real notebook retrieval."));
  S47J::Set(O,TEXT("limits"),TEXT("Development API controls, not human mouse/keyboard or subjective listening acceptance. The named incident and quiet initial scheduler are declared scenario fixtures, not natural random scheduling coverage. No teleport recovery within the journey. One positive pulsar discrepancy plus normal reference/negative internal control, not a full event matrix. Driver bound540s; runner must also verify normal process exit."));
  O->SetArrayField(TEXT("evidence"),S.Evidence);O->SetArrayField(TEXT("walkingRoute"),S.Route);
  if(!S.Root.IsEmpty()&&!FFileHelper::SaveStringToFile(S47J::Write(O,true),*(S.Root/TEXT("result.json"))))Good=false;
  UE_LOG(LogTemp,Display,TEXT("S47 Evidence %s stage%d checks%d walked%.2f: %s"),Good?TEXT("PASS"):TEXT("FAIL"),S.Stage,S.Checks,S.WalkMetres,*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
 }
 bool Check(US47Main* M,bool Good,const FString& Why)
 {
  ++S.Checks;auto O=S47J::New();S47J::Set(O,TEXT("check"),Why);S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("stage"),S.Stage);S.Evidence.Add(MakeShared<FJsonValueObject>(O));
  UE_LOG(LogTemp,Display,TEXT("S47 Evidence %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);if(!Good)Finish(M,false,Why);return Good;
 }
 void Photo(const TCHAR* Name){S.PendingPhoto=Name;S.PhotoAt=FPlatformTime::Seconds();}
 void Aim(US47Main* M,const FVector& Target)
 {const FVector D=Target-S47Three::ToThree(M->Player->EyeUE());M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
 FVector Target(US47Main* M,const FString& Id)
 {
  if(Id.StartsWith(TEXT("ff:audio:")))return M->Freeform->AudioDesk->InteractionPoint(Id);
  FS47Obj Obj;if(const auto* I=M->Inter->Get(Id))Obj=I->Object;
  if(!Obj)if(const auto* I=M->Web->Item(Id))Obj=M->Web->ObjAt(I->Node);
  if(!Obj)return FVector::ZeroVector;const FBox B=Obj.WorldBox();return B.IsValid?B.GetCenter():Obj.WorldPosition();
 }
 bool Use(US47Main* M,const FString& Id)
 {
  const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  const bool Good=Picked==Id&&bool(M->Player->OnUse);
  if(!Good)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("stage"),S.Stage);S47J::Set(O,TEXT("expected"),Id);S47J::Set(O,TEXT("picked"),Picked);S47J::Set(O,TEXT("position"),Vec(M->Player->Pos));S47J::Set(O,TEXT("floorY"),M->Player->FloorY);
   S47J::Set(O,TEXT("eyeThree"),Vec(S47Three::ToThree(M->Player->EyeUE())));S47J::Set(O,TEXT("forwardUE"),Vec(M->Player->ForwardUE()));S47J::Set(O,TEXT("targetThree"),Vec(Target(M,Id)));S47J::Set(O,TEXT("yaw"),M->Player->Yaw);S47J::Set(O,TEXT("pitch"),M->Player->Pitch);S47J::Set(O,TEXT("worldArea"),M->World->Area);S47J::Set(O,TEXT("webArea"),M->Web->Area());
   // Observe production picking without changing its label, range, ray or collision policy.
   S47J::Set(O,TEXT("labelOf"),M->Inter->LabelOf(Id));S47J::Set(O,TEXT("freeformLabel"),M->Freeform?M->Freeform->Label(Id):FString());
   const auto* Registered=M->Inter->Get(Id);const auto* Exported=M->Web->Item(Id);
   S47J::Set(O,TEXT("registered"),Registered!=nullptr);S47J::Set(O,TEXT("registeredNode"),Registered?Registered->Object.I:-1);
   S47J::Set(O,TEXT("registeredLabel"),Registered&&Registered->Label?Registered->Label():FString());
   S47J::Set(O,TEXT("registeredRange"),Registered?Registered->Range:-1.);S47J::Set(O,TEXT("exportedNode"),Exported?Exported->Node:-1);
   S47J::Set(O,TEXT("exportedRange"),Exported?Exported->Range:-1.);
   S47J::Set(O,TEXT("derivedEffectiveRange"),Registered&&Registered->Range>0?Registered->Range:(Exported?Exported->Range:2.4));
   auto Component=[&](UPrimitiveComponent* C)
   {
    auto D=S47J::New();if(!C)return D;
    S47J::Set(D,TEXT("path"),C->GetPathName());S47J::Set(D,TEXT("node"),M->Web->NodeOf(C));S47J::Set(D,TEXT("usable"),M->Web->UsableAt(C));
    S47J::Set(D,TEXT("registered"),C->IsRegistered());S47J::Set(D,TEXT("visible"),C->IsVisible());S47J::Set(D,TEXT("collision"),int32(C->GetCollisionEnabled()));
    S47J::Set(D,TEXT("useResponse"),int32(C->GetCollisionResponseToChannel(ECC_GameTraceChannel1)));S47J::Set(D,TEXT("visibilityResponse"),int32(C->GetCollisionResponseToChannel(ECC_Visibility)));
    S47J::Set(D,TEXT("boundsOriginUE"),Vec(C->Bounds.Origin));S47J::Set(D,TEXT("boundsExtentUE"),Vec(C->Bounds.BoxExtent));return D;
   };
   if(const auto* N=M->Web->Node(Registered?Registered->Object.I:-1))S47J::Set(O,TEXT("registeredComponent"),Component(Cast<UPrimitiveComponent>(N->Comp.Get())));
   if(const auto* N=M->Web->Node(Exported?Exported->Node:-1))S47J::Set(O,TEXT("exportedComponent"),Component(Cast<UPrimitiveComponent>(N->Comp.Get())));
   for(const ECollisionChannel Channel:{ECC_GameTraceChannel1,ECC_Visibility})
   {
    TArray<FHitResult> Hits;FCollisionQueryParams Params(SCENE_QUERY_STAT(S47EvidencePickFailure),true);
    M->Web->GetWorld()->LineTraceMultiByChannel(Hits,M->Player->EyeUE(),M->Player->EyeUE()+M->Player->ForwardUE().GetSafeNormal()*600.,Channel,Params);
    TArray<TSharedPtr<FJsonValue>> Rays;for(const auto& Hit:Hits)
    {
     auto D=Component(Hit.GetComponent());S47J::Set(D,TEXT("distanceMetres"),Hit.Distance/100.);S47J::Set(D,TEXT("blocking"),Hit.bBlockingHit);
     const FString HitId=M->Web->UsableAt(Hit.GetComponent());S47J::Set(D,TEXT("labelOf"),M->Inter->LabelOf(HitId));Rays.Add(MakeShared<FJsonValueObject>(D));
    }
    O->SetArrayField(Channel==ECC_Visibility?TEXT("visibilityRayHits"):TEXT("useRayHits"),Rays);
   }
   // Diagnostic snapshot only: never used to inject progress or replace the journey.
   FFileHelper::SaveStringToFile(S47J::Write(M->Flow->GameState(),true),*(S.Root/TEXT("failure-retained-state.json")));
   FFileHelper::SaveStringToFile(S47J::Write(O,true),*(S.Root/TEXT("failure-pick.json")));S.Evidence.Add(MakeShared<FJsonValueObject>(O));
   M->Player->TestMove.Reset();FScreenshotRequest::RequestScreenshot(S.Root/TEXT("failure-pick.png"),true,false);
   S.PickFailure=TEXT("actual camera ray failed for physical ")+Id+TEXT(" (picked ")+Picked+TEXT(")");S.PickFailureAt=FPlatformTime::Seconds();return false;
  }
  if(!Check(M,true,TEXT("actual camera ray picks physical ")+Id))return false;
  M->Player->OnUse(Id);return true;
 }
 void Close(US47Main* M){M->UI->Close(true);S.LockedSlider.Reset();}
 void WalkTo(US47Main* M,TArray<FVector2D> Points,int32 Next)
 {
  S.Path=MoveTemp(Points);S.Cursor=0;S.Walking=true;S.HaveFoot=true;S.LastFoot=S.ProgressFoot=M->Player->Pos;S.ProgressAt=FPlatformTime::Seconds();Go(Next);
 }
 bool Walk(US47Main* M,double Now)
 {
  auto* P=M->Player.Get();if(M->ModalOpen()){Finish(M,false,TEXT("walking route still has an open modal"));return false;}
  const double D=FVector2D(P->Pos.X-S.LastFoot.X,P->Pos.Z-S.LastFoot.Z).Size();
  if(D>.65){Finish(M,false,TEXT("Walker route jumped; no placement recovery is allowed"));return false;}S.WalkMetres+=D;S.LastFoot=P->Pos;
  if(FVector2D(P->Pos.X-S.ProgressFoot.X,P->Pos.Z-S.ProgressFoot.Z).Size()>.15){S.ProgressFoot=P->Pos;S.ProgressAt=Now;}
  if(Now-S.ProgressAt>8){Finish(M,false,TEXT("physical Walker route blocked for8 seconds"));return false;}
  if(Now-S.SampleAt>.15){S.SampleAt=Now;auto O=S47J::New();S47J::Set(O,TEXT("stage"),S.Stage);S47J::Set(O,TEXT("position"),Vec(P->Pos));S47J::Set(O,TEXT("eye"),Vec(S47Three::ToThree(P->EyeUE())));S.Route.Add(MakeShared<FJsonValueObject>(O));}
  if(S.Cursor>=S.Path.Num()){P->TestMove.Reset();S.Walking=false;S.At=Now;return true;}
  const FVector2D V=S.Path[S.Cursor]-FVector2D(P->Pos.X,P->Pos.Z);const double Distance=V.Size();
  if(Distance<.11){++S.Cursor;P->TestMove.Reset();return false;}
  P->Yaw=FMath::Atan2(-V.X,-V.Y);P->Pitch=-.06;P->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.4));return false;
 }
 bool Text(const TSharedRef<SWidget>& W,const FString& Needle,bool Exact)
 {
  if(!W->GetVisibility().IsVisible())return false;
  if(W->GetTypeAsString()==TEXT("STextBlock")){const FString T=StaticCastSharedRef<STextBlock>(W)->GetText().ToString();if(Exact?T==Needle:T.Contains(Needle))return true;}
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)if(Text(C->GetChildAt(I),Needle,Exact))return true;return false;
 }
 TArray<TSharedRef<SWidget>> Roots(US47Main* M)
 {
  TArray<TSharedRef<SWidget>> Out;TSet<const SWidget*> Seen;
  if(M->Freeform&&M->Freeform->TerminalOnGlass()&&FSlateApplication::IsInitialized())
   FSlateApplication::Get().ForEachUser([&](FSlateUser& User)
   {TSharedPtr<SWidget> W=User.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;
    if(W->GetTypeAsString().Contains(TEXT("Window"))&&!Seen.Contains(W.Get())){Seen.Add(W.Get());Out.Add(W.ToSharedRef());}},true);
  else if(M->Hud)if(const auto W=M->Hud->ModalWidget())Out.Add(W.ToSharedRef());
  return Out;
 }
 void Buttons(const TSharedRef<SWidget>& W,const FString& Label,bool Exact,TArray<TSharedRef<SButton>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetTypeAsString()==TEXT("SButton")&&Text(W,Label,Exact))Out.AddUnique(StaticCastSharedRef<SButton>(W));
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Buttons(C->GetChildAt(I),Label,Exact,Out);
 }
 int32 ButtonCount(US47Main* M,const FString& Label)
 {TArray<TSharedRef<SButton>> Found;for(const auto& W:Roots(M))Buttons(W,Label,true,Found);return Found.Num();}
 bool Click(US47Main* M,const FString& Label,bool Exact=true)
 {
  TArray<TSharedRef<SButton>> Found;for(const auto& W:Roots(M))Buttons(W,Label,Exact,Found);
  if(!Check(M,Found.Num()==1&&Found[0]->IsEnabled(),TEXT("actual unique enabled native Slate button: ")+Label+FString::Printf(TEXT(" (found %d)"),Found.Num())))return false;
#if !UE_BUILD_SHIPPING
  Found[0]->SimulateClick();return true;
#else
  Finish(M,false,TEXT("native Slate action test requires development build"));return false;
#endif
 }
 void Sliders(const TSharedRef<SWidget>& W,TArray<TSharedRef<SSlider>>& Out)
 {if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("SSlider"))Out.AddUnique(StaticCastSharedRef<SSlider>(W));auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Sliders(C->GetChildAt(I),Out);}
 void Knobs(const TSharedRef<SWidget>& W,const FString& Label,TArray<TSharedRef<SSlider>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetTypeAsString()==TEXT("SVerticalBox")&&Text(W,Label,true)){TArray<TSharedRef<SSlider>> Local;Sliders(W,Local);if(Local.Num()==1)Out.AddUnique(Local[0]);}
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Knobs(C->GetChildAt(I),Label,Out);
 }
 bool Tune(US47Main* M)
 {
  const auto* P=M->Freeform->ConsoleProfile();if(!P){Finish(M,false,TEXT("selected actual receiver card is unavailable"));return false;}
  const TCHAR* Names[]={TEXT("FREQUENCY MHz"),TEXT("GAIN"),TEXT("BANDWIDTH kHz")};
  const double Targets[]={FMath::RoundToDouble(P->F*1000.)/1000.,FMath::RoundToDouble((P->Gain[0]+P->Gain[1])*.5),FMath::RoundToDouble((P->Bw[0]+P->Bw[1])*.5)};
  const double Values[]={M->Story->Rx.Frequency,M->Story->Rx.Gain,M->Story->Rx.Bandwidth},Steps[]={.001,1,1};
  if(S.Knob>=3)return true;
  TArray<TSharedRef<SSlider>> Found;for(const auto& W:Roots(M))Knobs(W,Names[S.Knob],Found);
  if(Found.Num()!=1){Finish(M,false,FString(TEXT("unique real receiver slider unavailable: "))+Names[S.Knob]);return false;}
  const auto Slider=Found[0];
  if(S.LockedSlider.Pin()!=Slider){Slider->OnKeyDown(Slider->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));S.LockedSlider=Slider;}
  if(FMath::Abs(Values[S.Knob]-Targets[S.Knob])<=Steps[S.Knob]*.51)
  {Slider->OnKeyDown(Slider->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));S.LockedSlider.Reset();++S.Knob;return false;}
  Slider->OnNavigation(Slider->GetCachedGeometry(),FNavigationEvent(FModifierKeysState(),0,Values[S.Knob]<Targets[S.Knob]?EUINavigation::Right:EUINavigation::Left,ENavigationGenesis::Keyboard));return false;
 }
 bool Document(US47Main* M,const FString& Id,bool Retain)
 {
  const auto* Doc=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==Id;});
  if(!Check(M,Doc&&!Doc->Page.IsEmpty()&&!Doc->Stamp.IsEmpty(),TEXT("actual retained document exists: ")+Id))return false;
  if(Retain){S.Originals.Add({Id,Doc->Page,Doc->Stamp});return true;}
  const auto* Before=S.Originals.FindByPredicate([&](const FOriginal& D){return D.Id==Id;});
  return Check(M,Before&&Before->Page==Doc->Page&&Before->Stamp==Doc->Stamp,TEXT("immutable original/packet page and stamp retained byte-for-byte: ")+Id);
 }
 void CrossSample(US47Main* M)
 {
  const FVector2D Cross[]={{0,.5},{0,-.5},{.5,0},{-.5,0},{0,0}};if(S.Cross>=5)return;
  const FVector2D Command(M->Web->DishTargetAz(0)-S.BaseAz,M->Web->DishTargetEl(0)-S.BaseEl);
  if(FVector2D::Distance(Command,Cross[S.Cross])>.06)return;
  if(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(0),S.BaseAz+Command.X))>.1||FMath::Abs(M->Web->DishEl(0)-S.BaseEl-Command.Y)>.1)return;
  auto O=S47J::New();S47J::Set(O,TEXT("crossPoint"),S.Cross);S47J::Set(O,TEXT("actualAz"),M->Web->DishAz(0));S47J::Set(O,TEXT("actualEl"),M->Web->DishEl(0));S.Evidence.Add(MakeShared<FJsonValueObject>(O));++S.Cross;
 }
}
namespace S47FreeformEvidenceTest
{
 void Update(AS47Web* W)
 {
  if(!FParse::Param(FCommandLine::Get(),TEXT("S47FreeformEvidenceTest"))||S.Done)return;
  auto* M=W?US47Main::Get(W):nullptr;if(!M)return;const double Now=FPlatformTime::Seconds();
  if(!S.PickFailure.IsEmpty())
  {
   // Keep the failed camera pose intact until the renderer has produced evidence.
   if(Now-S.PickFailureAt<.5)return;
   if(IFileManager::Get().FileSize(*(S.Root/TEXT("failure-pick.png")))<1024&&Now-S.PickFailureAt<2)return;
   Check(M,false,S.PickFailure);return;
  }
  if(S.Started&&Now-S.Began>540){Finish(M,false,TEXT("bounded540-second evidence journey timeout"));return;}
  if(!M->Flow||!M->World||!M->Player||!M->Inter||!M->Story||!M->UI||M->Flow->bStarting||S47Startup::Blocking())return;
  if(!S.Started)
  {
   // Props can tick before Flow/HUD have finished selecting/constructing the session.
   // Readiness is a wait; exact isolated-case validation still happens before mutation.
   if(M->Mode!=US47Main::EMode::Play||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!M->Freeform->Field||!M->Freeform->AudioDesk||!M->Freeform->AudioDesk->Ready()||!W->IsLoaded()||M->World->bBusy)return;
   FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
   if(!User.StartsWith(TEXT("/tmp/"))||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),FPaths::ConvertRelativePathToFull(User))||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!M->Flow->bDevRun||M->Flow->CaseId!=0){Finish(M,false,TEXT("explicit isolated fresh Freeform developer case0 required"));return;}
   S.Started=true;S.Began=Now;S.Root=FPaths::ProjectSavedDir()/TEXT("FreeformEvidenceTest");IFileManager::Get().MakeDirectory(*S.Root,true);Go(0,2);return;
  }
  if(!S.PendingPhoto.IsEmpty())
  {if(Now-S.PhotoAt<.35)return;S.Photos.Add(S.PendingPhoto);FScreenshotRequest::RequestScreenshot(S.Root/(S.PendingPhoto+TEXT(".png")),true,false);S.PendingPhoto.Empty();S.At=Now;return;}
  if(S.Walking){Walk(M,Now);return;}
  if(Now-S.At<S.Delay)return;
  if(S.Stage==70)
  {
   if(M->Mode!=US47Main::EMode::Title)return;
   if(!Check(M,M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue action exists")))return;Go(71,1);return;
  }
  if(M->Mode!=US47Main::EMode::Play||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||M->World->bBusy)return;
  auto& F=*M->Freeform;const auto* J=F.CurrentJob();
  if(S.Stage==0)
  {
   if(!Check(M,F.Director&&F.AudioDesk&&F.AudioDesk->Ready()&&F.HasPowerForReactiveLight()&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&F.Debt()==0,TEXT("actual powered SARO instruments available with fresh zero-debt observing session")))return;
   for(const auto& Job:F.Jobs())if(Job.Day==F.Day()&&Job.Source<2&&Job.Stage==EJob::New)S.Ids[Job.Source]=Job.Id;
   if(!Check(M,!S.Ids[0].IsEmpty()&&!S.Ids[1].IsEmpty(),TEXT("two genuine unfinished daily source0/source1 jobs exist")))return;
   S.InitialCash=F.Budget();S.InitialDebt=F.Debt();
   auto Director=F.Director->Save();S47J::Set(Director,TEXT("seenDay"),F.Day());S47J::Set(Director,TEXT("nextAt"),S47J::Num(Director,TEXT("activeTime"))+1200.);F.Director->Begin(Director);
   FFileHelper::SaveStringToFile(S47J::Write(Director,true),*(S.Root/TEXT("declared-quiet-scheduler-fixture.json")));
   M->Player->Place(-4.2,2.49,0);M->Player->bHeadMotion=false;Go(100);return;
  }
  if(S.Stage==100){Aim(M,Target(M,TEXT("opsTerminal")));Go(1);return;}
  if(S.Stage==1){if(!Use(M,TEXT("opsTerminal")))return;Go(2,.6);return;}
  if(S.Stage==2)
  {if(!Check(M,F.TerminalOnGlass()&&F.TerminalWindowRendered(),TEXT("real OPS window renders on its native glass"))||!Click(M,S.Ids[S.Job],false))return;Go(3);return;}
  if(S.Stage==3)
  {
   if(!Check(M,J&&J->Id==S.Ids[S.Job]&&J->Stage==EJob::New,TEXT("real daily-work Slate action selects the intended unfinished source")))return;
   Close(M);
   if(S.Job==0)WalkTo(M,{{-4.2,0},{-2.14,0},{-2.14,-2.15}},4);
   else
   {
    if(!Check(M,F.Director->Force(TEXT("extra_pulse")),TEXT("declared named extra_pulse event fixture starts after the measured normal reference")))return;
    WalkTo(M,{{-4.2,0},{2.16,0},{2.16,-2.15}},6);
   }
   return;
  }
  if(S.Stage==4){Aim(M,Target(M,TEXT("crtLeft")));Go(5);return;}
  if(S.Stage==5)
  {if(!Use(M,TEXT("crtLeft")))return;Close(M);WalkTo(M,{{-2.14,-1.7},{2.16,-1.7},{2.16,-2.15}},6);return;}
  if(S.Stage==6){Aim(M,Target(M,TEXT("crtRight")));Go(7);return;}
  if(S.Stage==7){if(!Use(M,TEXT("crtRight")))return;Go(8,.6);return;}
  if(S.Stage==8)
  {
   const auto* P=F.ConsoleProfile();if(!Check(M,P&&F.TerminalWindowRendered(),TEXT("actual tracking terminal/card is ready")))return;
   S.BaseAz=(P->Az[0]+P->Az[1])*.5;S.BaseEl=J->bGenerated?J->Card.El:FS47Observing::Runs()[J->Source].El;
   if(!Click(M,TEXT("Point array to selected source")))return;Close(M);WalkTo(M,{{2.16,-2.15},{0,-2.15}},9);return;
  }
  if(S.Stage==9){Aim(M,Target(M,TEXT("crtCenter")));Go(10);return;}
  if(S.Stage==10){if(!Use(M,TEXT("crtCenter")))return;S.Knob=0;Go(11,.6);return;}
  if(S.Stage==11)
  {
   if(Now-S.At>60){Finish(M,false,TEXT("receiver's actual native slider callbacks did not converge"));return;}
   if(!Tune(M))return;
   if(!Check(M,M->Story->Rx.Pass(*F.ConsoleProfile()),TEXT("real receiver UI slider navigation reaches the actual card's acceptance window")))return;Go(12);return;
  }
  if(S.Stage==12)
  {
   TArray<TSharedRef<SButton>> Ready;for(const auto& RootWidget:Roots(M))Buttons(RootWidget,TEXT("READY / START ACQUISITION"),true,Ready);
   if(Ready.Num()!=1||!Ready[0]->IsEnabled())return;
   if(!Click(M,TEXT("READY / START ACQUISITION")))return;
   if(!Check(M,J->Stage==EJob::Integrating&&(!S.Job||(J->bAnomaly&&J->Phenomenon==TEXT("extra_pulse"))),TEXT("production acquisition starts only on settled array and captures the established incident identity")))return;Go(13,0);return;
  }
  if(S.Stage==13)
  {
   if(!S.Job)CrossSample(M);
   if(J->Stage!=EJob::Chart)return;
   if(!Check(M,S.Job?J->Elapsed==18:S.Cross==5,TEXT("actual timed acquisition completes; normal reference includes all five settled cross positions")))return;
   Close(M);WalkTo(M,{{0,-1.7},{3.8,-1.7},{4.8,-.4}},14);return;
  }
  if(S.Stage==14){Aim(M,Target(M,TEXT("printer")));Go(15);return;}
  if(S.Stage==15)
  {
   if(!Use(M,TEXT("printer"))||!Check(M,J->Stage==EJob::Recorded,TEXT("actual printer use collects the uncollected original chart"))||!Document(M,J->Id+TEXT("-original"),true))return;
   Photo(S.Job?TEXT("02-pulsar-original"):TEXT("01-normal-reference-original"));Go(16);return;
  }
  if(S.Stage==16)
  {
   Close(M);
   if(!S.Job)WalkTo(M,{{4.4,-.4},{3.8,1.2},{-4.2,1.2},{-4.2,2.49}},30);
   else WalkTo(M,{{4.4,-1.3},{4.4,-2.24},{4.7,-2.24}},17);return;
  }
  if(S.Stage==17){Aim(M,Target(M,TEXT("rack")));Go(18);return;}
  if(S.Stage==18)
  {if(!Use(M,TEXT("rack"))||!Check(M,J->bControl&&J->ControlAt<8,TEXT("actual receiver rack starts an independent control without altering the original")))return;Go(19,0);return;}
  if(S.Stage==19)
  {
   if(J->ControlAt<8)return;
   if(!Document(M,J->Id+TEXT("-original"),false))return;
   const FVector A=F.AudioDesk->InteractionPoint(TEXT("ff:audio:original"));WalkTo(M,{{4.4,-1.3},{3.8,1.2},{A.X,1.2},{A.X,A.Z-1.2}},20);return;
  }
  if(S.Stage==20){if(!Check(M,F.AudioDesk->ListeningPosition(),TEXT("actual walking reaches the powered physical tape listening position")))return;Aim(M,Target(M,TEXT("ff:audio:original")));Go(21);return;}
  if(S.Stage==21){if(!Use(M,TEXT("ff:audio:original")))return;Aim(M,Target(M,TEXT("ff:audio:play")));Go(22);return;}
  if(S.Stage==22){if(!Use(M,TEXT("ff:audio:play"))||!Check(M,F.AudioPlaying()&&!M->ModalOpen(),TEXT("physical PLAY begins the original recording while world remains active")))return;Go(23,0);return;}
  if(S.Stage==23)
  {if(F.AudioPosition()<8.7)return;Aim(M,Target(M,TEXT("ff:audio:mark_start")));Go(24,.15);return;}
  if(S.Stage==24)
  {if(F.AudioPosition()<9.1)return;if(!Use(M,TEXT("ff:audio:mark_start"))||!Check(M,J->MarkStart>=9.1&&J->MarkStart<=9.3,TEXT("physical MARK IN captures actual original cursor near9.1s")))return;Go(25,0);return;}
  if(S.Stage==25)
  {if(F.AudioPosition()<12.2)return;Aim(M,Target(M,TEXT("ff:audio:mark_end")));Go(26,.15);return;}
  if(S.Stage==26)
  {if(F.AudioPosition()<12.8)return;if(!Use(M,TEXT("ff:audio:mark_end"))||!Check(M,J->MarkEnd>=12.6&&J->MarkEnd<=13.1,TEXT("physical MARK OUT captures both extra tick groups")))return;Go(27,0);return;}
  if(S.Stage==27)
  {if(F.AudioPlaying())return;if(!Check(M,J->OriginalCovered==18,TEXT("original is actually heard in full; no seek/coverage injection")))return;Aim(M,Target(M,TEXT("ff:audio:control")));Go(28);return;}
  if(S.Stage==28){if(!Use(M,TEXT("ff:audio:control")))return;Aim(M,Target(M,TEXT("ff:audio:play")));Go(29);return;}
  if(S.Stage==29){if(!Use(M,TEXT("ff:audio:play")))return;Go(290,0);return;}
  if(S.Stage==290)
  {if(F.AudioPlaying())return;if(!Check(M,J->ControlCovered==8&&!J->bAudioReviewed,TEXT("independent negative control is actually heard for8s before interpretation")))return;Aim(M,Target(M,TEXT("ff:audio:retain")));Go(291);return;}
  if(S.Stage==291)
  {if(!Use(M,TEXT("ff:audio:retain"))||!Check(M,J->bAudioReviewed&&F.HasFinding(TEXT("pulse-review"))&&F.Budget()==S.InitialCash+20,TEXT("physical RETAIN secures comparison without payment or origin claim")))return;Photo(TEXT("03-physical-original-control"));Go(292);return;}
  if(S.Stage==292){WalkTo(M,{{.42,1.2},{-4.2,1.2},{-4.2,2.49}},30);return;}
  if(S.Stage==30){Aim(M,Target(M,TEXT("opsTerminal")));Go(31);return;}
  if(S.Stage==31){if(!Use(M,TEXT("opsTerminal")))return;Go(32,.6);return;}
  if(S.Stage==32)
  {
   if(!S.Job)
   {
    if(!Click(M,TEXT("Register routine result / $20"))||!Check(M,J->Stage==EJob::Filed&&!J->bAnomaly&&F.Budget()==S.InitialCash+20&&!F.HasFinding(TEXT("pulse-review")),TEXT("normal reference files once, pays20 and creates no false discrepancy")))return;
    S.Job=1;Go(2);return;
   }
   if(!Click(M,TEXT("The quiet control proves a sky source"))||!Check(M,J->Stage==EJob::Recorded&&F.Budget()==S.InitialCash+20,TEXT("unsupported sky-origin interpretation is refused without credit or stage advance")))return;Go(33);return;
  }
  if(S.Stage==33)
  {if(!Click(M,TEXT("Source unresolved; retain both measurements"))||!Check(M,J->Stage==EJob::Registered&&F.Budget()==S.InitialCash+20,TEXT("supported unresolved registration retains original and awaits physical filing")))return;Go(330,.6);return;}
  if(S.Stage==330)
  {
   const auto* C=F.CaseFile().Get(0);
   if(!Click(M,TEXT("Case board")))return;
   if(!Check(M,C&&C->bOpen&&!C->bConcluded&&C->Planned<0,TEXT("the reviewed extra-pulse original opens the interval case")))return;Go(331,.8);return;
  }
  if(S.Stage==331)
  {
   bool Board=false;for(const auto& Root:Roots(M))Board|=Text(Root,TEXT("THE REPEATED INTERVAL"),false)&&Text(Root,TEXT("STILL POSSIBLE"),false)&&Text(Root,TEXT("PREDICTION"),false);
   if(!Check(M,Board,TEXT("actual case board shows the question, three open explanations and test predictions")))return;Photo(TEXT("03b-case-board"));Go(332,1.2);return;
  }
  if(S.Stage==332)
  {
   const int32 Before=F.Jobs().Num();
   if(!Click(M,TEXT("Plan this test: Generator power")))return;
   const auto* C=F.CaseFile().Get(0);
   if(!Check(M,C&&C->Planned==0&&F.Jobs().Num()==Before+1&&F.Jobs().Last().CaseTest==0&&F.CurrentJob()&&F.CurrentJob()->Id==S.Ids[1],TEXT("planning a case test adds its own job without stealing the registered packet's selection")))return;Go(333,.8);return;
  }
  if(S.Stage==333)
  {
   bool Planned=false;for(const auto& Root:Roots(M))Planned|=Text(Root,TEXT("PLANNED"),false);
   if(!Check(M,Planned,TEXT("actual case board marks the generator test as planned")))return;Photo(TEXT("03c-case-test-planned"));Go(334,1.2);return;
  }
  if(S.Stage==334){if(!Click(M,TEXT("Back to daily work")))return;Go(335,.8);return;}
  if(S.Stage==335)
  {
   bool Plan=false;for(const auto& Root:Roots(M))Plan|=Text(Root,TEXT("TONIGHT'S PLAN"),false)&&Text(Root,TEXT("GENERATOR"),false);
   if(!Check(M,Plan,TEXT("OPS daily work shows tonight's plan with generator readiness")))return;Photo(TEXT("03d-tonights-plan"));Go(336,1.2);return;
  }
  if(S.Stage==336){Close(M);WalkTo(M,{{-4.2,1.2},{3.8,1.2},{4.8,-.4}},34);return;}
  if(S.Stage==34){Aim(M,Target(M,TEXT("printer")));Go(35);return;}
  if(S.Stage==35)
  {if(!Use(M,TEXT("printer"))||!Check(M,J->Stage==EJob::Packet,TEXT("actual printer use collects the registered paper packet"))||!Document(M,J->Id+TEXT("-packet"),true)||!Document(M,J->Id+TEXT("-original"),false))return;Photo(TEXT("04-registered-packet"));Go(36);return;}
  if(S.Stage==36){Close(M);WalkTo(M,{{3.8,1.2},{-2.4,1.2},{-2.4,3.6}},37);return;}
  if(S.Stage==37){Aim(M,Target(M,TEXT("doorSouth")));Go(38);return;}
  if(S.Stage==38)
  {if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Use(M,TEXT("doorSouth")))return;WalkTo(M,{{-2.4,5.7},{-3.9,5.7}},39);return;}
  if(S.Stage==39){Aim(M,Target(M,TEXT("recordsDoor")));Go(40);return;}
  if(S.Stage==40)
  {if(!M->Web->IsDoorOpen(TEXT("recordsDoor"))&&!Use(M,TEXT("recordsDoor")))return;WalkTo(M,{{-3.9,7.5},{-2.3,7.6},{-2.3,9.6},{-2.2,9.6}},41);return;}
  if(S.Stage==41){Aim(M,Target(M,TEXT("binder")));Go(42);return;}
  if(S.Stage==42){if(!Use(M,TEXT("binder")))return;Go(43,.5);return;}
  if(S.Stage==43)
  {if(!Click(M,TEXT("Place registered originals in this binder"))||!Check(M,J->Stage==EJob::Filed&&J->bPaid&&F.HasFinding(TEXT("pulse-archive"))&&F.Budget()==S.InitialCash+40,TEXT("actual binder filing pays exactly once after both physical printer collections")))return;Photo(TEXT("05-physically-filed-archive"));Go(44);return;}
  if(S.Stage==44)
  {if(!Check(M,ButtonCount(M,TEXT("Place registered originals in this binder"))==0,TEXT("filed archive offers no repeat filing/payment action")))return;Close(M);Go(60,.5);return;}
  if(S.Stage==60)
  {
   if(!S.Quarantined)
   {M->Flow->Saves=FS47SaveStore(S.Root/TEXT("SaveGames"));M->Flow->Saves.Init();M->Flow->CaseId=1;M->Flow->bDevRun=false;M->Flow->Saves.SetActive(1);S.Quarantined=true;}
   if(!M->Flow->SaveBlock().IsEmpty()){if(Now-S.At>8)Finish(M,false,TEXT("ordinary save boundary refused: ")+M->Flow->SaveBlock());return;}
   for(const auto& Doc:S.Originals)if(!Document(M,Doc.Id,false))return;
   S.SavedFoot=M->Player->Pos;FString Error;const auto Written=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Choice=Reader.ContinueSave();const auto Disk=Choice?Reader.Load(Choice->Id):nullptr;
   if(!Check(M,Written&&Choice&&Disk&&Choice->Id==Written->Id&&S47J::Str(Disk,TEXT("sessionMode"))==TEXT("freeform"),TEXT("ordinary WriteSave and fresh disk Continue reader select this complete Freeform case: ")+Error))return;
   FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(S.Root/TEXT("actual-continue-disk.json")));M->Flow->ToTitle();Go(70,.5);return;
  }
  if(S.Stage==71)
  {
   if(!Check(M,M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->World->Area==TEXT("saro")&&FVector::Dist(M->Player->Pos,S.SavedFoot)<.03&&!M->ModalOpen()&&!F.TerminalOnGlass(),TEXT("ordinary title/disk Continue restores actual records-room pose, Freeform mode and no orphan modal")))return;
   if(!Check(M,J&&J->Id==S.Ids[1]&&J->Stage==EJob::Filed&&J->bPaid&&J->bAudioReviewed&&J->OriginalCovered==18&&J->ControlCovered==8&&J->MarkStart<=9.3&&J->MarkEnd>=12.6&&F.Budget()==S.InitialCash+40&&F.Debt()==S.InitialDebt,TEXT("disk Continue preserves measured comparison, original marks and exactly-once filing credit")))return;
   if(!Check(M,F.CaseFile().Get(0)&&F.CaseFile().Get(0)->bOpen&&F.CaseFile().Get(0)->Planned==0&&F.Jobs().ContainsByPredicate([](const FS47FreeformSession::FJob& X){return X.CaseTest==0&&!X.bCaseRecorded;}),TEXT("disk Continue keeps the open case and its planned generator test job")))return;
   for(const auto& Doc:S.Originals)if(!Document(M,Doc.Id,false))return;++S.Restores;Aim(M,Target(M,TEXT("binder")));Go(72);return;
  }
  if(S.Stage==72){if(!Use(M,TEXT("binder")))return;Go(73,.5);return;}
  if(S.Stage==73)
  {
   bool Found=false;for(const auto& Root:Roots(M))Found|=Text(Root,S.Ids[1]+TEXT("-original"),false)&&Text(Root,TEXT("SOURCE UNRESOLVED"),false)&&Text(Root,TEXT("FILED"),false);
   if(!Check(M,Found&&ButtonCount(M,TEXT("Place registered originals in this binder"))==0,TEXT("actual restored binder archive displays the same original reference, unresolved conclusion and filed state")))return;
   Photo(TEXT("06-continued-filed-archive"));Go(74);return;
  }
  if(S.Stage==74){Close(M);M->UI->NotebookAt(TEXT("papers"));M->Flow->OpenNotebook();Go(75,.6);return;}
  if(S.Stage==75)
  {
   const FString Title=TEXT("Original observation: ")+S.Ids[1];if(!Click(M,Title,false))return;Go(76);return; // unread papers carry a NEW / prefix
  }
  if(S.Stage==76)
  {
   // Notebook entry's first click selects; its second click opens the actual retained paper.
   if(const auto Modal=M->Hud->ModalWidget();Modal&&Modal->GetTypeAsString()==TEXT("SS47Notebook")){if(!Click(M,TEXT("Original observation: ")+S.Ids[1],false))return;}
   Go(77);return;
  }
  if(S.Stage==77)
  {
   const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;
   if(!Check(M,Modal&&Modal->GetTypeAsString()==TEXT("SS47Document"),TEXT("actual notebook paper action reopens the retained original document after Continue"))||!Document(M,S.Ids[1]+TEXT("-original"),false))return;
   Photo(TEXT("07-original-retrieved-after-continue"));Go(78);return;
  }
  if(S.Stage==78)Finish(M,true,TEXT("normal reference and pulsar/control evidence journey completed with physical filing and ordinary disk Continue"));
 }
}
