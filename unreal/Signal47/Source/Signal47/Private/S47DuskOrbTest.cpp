#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "S47FreeformTest.h"
#include "S47DuskOrb.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Night.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "UnrealClient.h"
#include "S47Image.h"
#include "Components/MeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 bool OrbButtonText(const TSharedRef<SWidget>& W,const FString& Label)
 {
  if(!W->GetVisibility().IsVisible())return false;
  if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label)return true;
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)if(OrbButtonText(C->GetChildAt(I),Label))return true;return false;
 }
 void OrbButtons(const TSharedRef<SWidget>& W,const FString& Label,TArray<TSharedRef<SButton>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetTypeAsString()==TEXT("SButton")&&OrbButtonText(W,Label))Out.AddUnique(StaticCastSharedRef<SButton>(W));
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)OrbButtons(C->GetChildAt(I),Label,Out);
 }
}
// Explicit dev-run and player-placement fixtures. Native rendering/exposure,
// physical lab callbacks and real disk/Flow restoration are not substituted.
void FS47FreeformTest::StepDuskOrb(double Dt)
{
 auto& F=*M->Freeform;
 auto NativeButtons=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Buttons;TSet<const SWidget*> Seen;
  if(const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr){Seen.Add(Modal.Get());OrbButtons(Modal.ToSharedRef(),Label,Buttons);}
  // On-glass terminals live in a separate Slate window, as in OpsNavigationTest.
  if(F.TerminalOnGlass()&&FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& User)
  {
   TSharedPtr<SWidget> W=User.GetFocusedWidget();if(!W)return;
   while(auto Parent=W->GetParentWidget())W=Parent;
   if(W->GetTypeAsString().Contains(TEXT("Window"))&&!Seen.Contains(W.Get())){Seen.Add(W.Get());OrbButtons(W.ToSharedRef(),Label,Buttons);}
  },true);
  return Buttons;
 };
 auto NoiseHints=[&]
 {
  TArray<TSharedRef<SWidget>> Found;TSet<const SWidget*> Seen;
  TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> W)
  {
   if(Seen.Contains(&W.Get())||!W->GetVisibility().IsVisible())return;Seen.Add(&W.Get());
   if(W->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(TEXT("OPTIONAL / LATER LOCAL-NOISE CHECK")))Found.Add(W);
   auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Visit(C->GetChildAt(I));
  };
  if(const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr)Visit(Modal.ToSharedRef());
  if(F.TerminalOnGlass()&&FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& U){auto W=U.GetFocusedWidget();if(!W)return;while(auto P=W->GetParentWidget())W=P;if(W->GetTypeAsString().Contains(TEXT("Window")))Visit(W.ToSharedRef());},true);
  return Found;
 };
 auto HintScroll=[](TSharedRef<SWidget> Hint)
 {
  TSharedPtr<SWidget> W=Hint;while(W){if(W->GetTypeAsString()==TEXT("SScrollBox"))return StaticCastSharedPtr<SScrollBox>(W);W=W->GetParentWidget();}
  return TSharedPtr<SScrollBox>();
 };
 auto NotebookText=[&]
 {
  FString Text;TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> W){if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("STextBlock"))Text+=StaticCastSharedRef<STextBlock>(W)->GetText().ToString()+TEXT("\n");auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Visit(C->GetChildAt(I));};
  if(auto W=M->Hud->ModalWidget())Visit(W.ToSharedRef());return Text;
 };
 auto NotebookFollowing=[&]
 {
  int32 Count=0;TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> W){if(!W->GetVisibility().IsVisible())return;if(W->GetTypeAsString()==TEXT("STextBlock")){const FString T=StaticCastSharedRef<STextBlock>(W)->GetText().ToString();if(T.StartsWith(TEXT("Following /"))&&!T.Contains(TEXT("\n")))++Count;}auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)Visit(C->GetChildAt(I));};
  if(auto W=M->Hud->ModalWidget())Visit(W.ToSharedRef());return Count;
 };
 auto NotebookButtons=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;if(auto W=M->Hud->ModalWidget())OrbButtons(W.ToSharedRef(),Label,Found);return Found;
 };
 auto NotebookClick=[&](const FString& Label)
 {
  // Notebook-local Click builds SButton; invoke that native callback, not OS input.
  const auto Found=NotebookButtons(Label);if(Found.Num()!=1||!Found[0]->IsEnabled())return false;Found[0]->SimulateClick();return true;
 };
 auto RestoreNotebook=[&]
 {
  if(auto Real=S47J::Obj(Snapshot,TEXT("notebookRealSession"))){M->UI->Close(true);F.Begin(S47J::Clone(Real));M->Night->Album->Restore(S47J::Clone(S47J::Obj(Snapshot,TEXT("notebookRealAlbum"))));M->RequestAutosave();}
 };
 auto NotebookCheck=[&](bool Good,const TCHAR* Why){if(!Good)RestoreNotebook();return Check(Good,Why);};
 auto RestoreFocusFixture=[&]
 {
  const auto Real=S47J::Obj(Snapshot,TEXT("nativeFocusRealDirector"));
  if(Real){M->UI->Close(true);const auto Album=S47J::Obj(Snapshot,TEXT("nativeFocusRealAlbum"));if(Album)M->Night->Album->Restore(S47J::Clone(Album));F.Director->Begin(S47J::Clone(Real));M->RequestAutosave();}
 };
 auto FocusCheck=[&](bool Good,const TCHAR* Why){if(!Good)RestoreFocusFixture();return Check(Good,Why);};
 if(Age>(FParse::Param(FCommandLine::Get(),TEXT("S47NotebookFocusTest"))?190:150)){RestoreNotebook();RestoreFocusFixture();Finish(false,TEXT("bounded dusk-orb native test timeout"));return;}
 auto Original=[&]()->const FS47AlbumShot*{return M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});};
 auto Ops=[&]{const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X+1.,P.Z,PI/2);}; // OPS is on the west wall, facing +X into the room.
 auto AimAt=[&](const FVector& Point){const FVector D=Point-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());};
 auto OrbNow=[&]{return FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());};
 auto OrbSchedule=[&]{return S47J::Obj(F.Director->Save(),TEXT("duskOrb"));};
 // Mean brightness of what the player sees in the lower middle of the picture, from the actual back buffer. -1 if it cannot be read.
 // The picture as written: reading the viewport back on the game thread gives only black on Vulkan.
 auto PictureMean=[](const FString& Path)
 {
  TArray<uint8> Bytes,Raw;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return -1.;
  auto Wrapper=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper")).CreateImageWrapper(EImageFormat::PNG);
  if(!Wrapper.IsValid()||!Wrapper->SetCompressed(Bytes.GetData(),Bytes.Num())||!Wrapper->GetRaw(ERGBFormat::BGRA,8,Raw))return -1.;
  const int64 Wd=Wrapper->GetWidth(),Ht=Wrapper->GetHeight();if(Wd<1||Ht<4||Raw.Num()<Wd*Ht*4)return -1.;
  double Sum=0;int64 Samples=0;
  for(int64 Y=Ht*35/100;Y<Ht*85/100;++Y)for(int64 X=0;X<Wd;++X){const uint8* Px=&Raw[(Y*Wd+X)*4];Sum+=.0722*Px[0]+.7152*Px[1]+.2126*Px[2];++Samples;}
  return Samples?Sum/Samples:-1.;
 };
 if(Stage==0)
 {
  if(Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->FieldCamera&&M->Night&&M->Night->Album&&F.Director,TEXT("orb harness requires isolated development profile and native photographic systems")))return;
  M->UI->Close(true);
  if(!M->FieldCamera->bHave){M->Player->Place(17.25,-1.9,0);M->Inter->UseId(TEXT("fieldCamera"));}
  if(!Check(M->FieldCamera->bHave,TEXT("orb fixture collects camera through physical lab callback")))return;
  // -S47DuskOrbNight: the lights outside the dusk window, from the scheduler's own clock. Never Force.
  if(FParse::Param(FCommandLine::Get(),TEXT("S47DuskOrbNight")))
  {
   F.Director->CancelForTest();F.TestEndFirstShift();F.Director->TestOrbNight(true);M->UI->ClearToasts();
   Snapshot=S47J::New();PhotoBudget=F.Budget();Go(9900,.4);return;
  }
  F.Director->CancelForTest();const double Hour=FMath::Fmod(F.Clock(),86400.);
  if(!Check(F.TestAdvance(FMath::Fmod(18.75*3600.-Hour+86400.,86400.))&&F.Director->Force(TEXT("dusk_orb")),TEXT("explicit dusk clock and Force start native orb; natural scheduling is covered separately")))return;
  PhotoBudget=F.Budget();Go(9500,4);return;
 }
 if(Stage==9500)
 {
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb")&&F.Director->PhysicalObjectPresented(),TEXT("dusk orb has a presented native component after gradual appearance")))return;
  Snapshot=S47J::New();S47J::Set(Snapshot,TEXT("age"),F.Director->EventAge());const auto P=F.Director->EventAnchor();S47J::Set(Snapshot,TEXT("anchor"),TArray<double>{P.X,P.Y,P.Z});
  if(!Restore(TEXT("orb-active")))return;Go(9501,.5);return;
 }
 if(Stage==9501)
 {
  const auto A=S47J::Nums(Snapshot,TEXT("anchor"));const double Before=S47J::Num(Snapshot,TEXT("age"));
  if(!Check(A.Num()==3&&F.Director->ActiveEvent()==TEXT("dusk_orb")&&F.Director->EventAnchor().Equals(FVector(A[0],A[1],A[2]),.001)&&F.Director->EventAge()>=Before&&F.Director->EventAge()<Before+3,TEXT("active Continue retains exact world anchor and elapsed path instead of resetting appearance")))return;
  // Existing native array-shot window position, inside the control room.
  // Only the viewer is placed; the orb continues its saved world-space path.
  M->Player->Place(-.9,-3.55,0);
  const FVector WindowTarget=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());
  const FVector WindowD=WindowTarget-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-WindowD.X,-WindowD.Z);M->Player->Pitch=FMath::Atan2(WindowD.Y,FVector2D(WindowD.X,WindowD.Z).Size());
  M->UI->ClearToasts();Go(95011,2.4);return;
 }
 if(Stage==95011)
 {
  // Visual invitation evidence only: real window/antenna occlusion is allowed.
  Shot(TEXT("orb-control-room-window"));Go(95012,.4);return;
 }
 if(Stage==95012)
 {
  const FVector P=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());M->Player->Place(P.X,P.Z+9,0);
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());M->UI->ClearToasts();Go(9502,2.3);return;
 }
 if(Stage==9502)
 {
  // The orb has moved on since the placement: aim at where it is now, as a person would.
  const FVector Live=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());const FVector LiveD=Live-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-LiveD.X,-LiveD.Z);M->Player->Pitch=FMath::Atan2(LiveD.Y,FVector2D(LiveD.X,LiveD.Z).Size());
  Go(95020,.12);return;
 }
 if(Stage==95020)
 {
  Shot(TEXT("orb-native-before-exposure"));Go(95021,.05);return;
 }
 if(Stage==95021)
 {
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
  if(!Check(!Photo.bUnavailable&&!Photo.Url.IsEmpty()&&F.Director->PhotoClaims().Num()==1&&F.Budget()==PhotoBudget,TEXT("actual rendered orb exposure qualifies one unpaid claim without synthetic image data")))return;
  PhotoClaimId=F.Director->PhotoClaims()[0].Id;RetainedPhotoId=Photo.Id;RetainedPhotoUrl=Photo.Url;
  if(!Check(PhotoClaimId.StartsWith(TEXT("dusk_orb."))&&Photo.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& P){return P.Key==TEXT("freeform:")+PhotoClaimId;}),TEXT("real exposure carries this orb incident's visibility-qualified photographic target")))return;
  S47J::Set(Snapshot,TEXT("photoClock"),Photo.Clock);Ops();Go(9503,.3);return;
 }
 if(Stage==9503)
 {
  if(!Check(!F.Director->PlanLocalNoiseCheck(PhotoClaimId)&&!F.Director->PlanLocalNoiseCheck(TEXT("missing-photo-incident")),TEXT("optional noise idea rejects undeveloped photo and absent incident")))return;
  if(!Check(F.CanRegisterPhotoEvidence()&&Original()&&Original()->LabStage==0&&!F.Director->PlanPhotoControl(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget,TEXT("physical OPS rejects control plan and payment while real orb original is undeveloped")))return;
  M->UI->Close(true);M->Web->SetDoorOpen(TEXT("lab"),false,true);const auto* Bench=M->Inter->Get(TEXT("wetBench"));
  if(!Check(Bench&&bool(Bench->Object),TEXT("orb print uses actual wet-bench world object")))return;
  const FVector P=Bench->Object.WorldPosition();M->Player->Place(P.X,P.Z+1,0);Go(9504,.3);return;
 }
 if(Stage==9504)
 {
  const auto* P=Original();if(!Check(P&&P->Url==RetainedPhotoUrl&&P->Clock==S47J::Num(Snapshot,TEXT("photoClock")),TEXT("lab retains actual orb image and original exposure timestamp")))return;
  if(P->IsDeveloped()){M->UI->ClearToasts();Go(95041,.4);return;}
  if(!M->Hud||!M->Hud->ModalWidget())M->Inter->UseId(TEXT("wetBench"));Go(9505,.2);return;
 }
 if(Stage==95041)
 {
  Shot(TEXT("orb-developed-original"));Go(95042,.4);return;
 }
 if(Stage==95042)
 {
  M->UI->Close(true);Ops();Go(9506,.3);return;
 }
 if(Stage==9505)
 {
  const auto* P=Original();if(!Check(P&&P->LabStage>=0&&P->LabStage<3,TEXT("orb original has one pending physical development stage")))return;
  const int32 Before=P->LabStage;const FString Label=Before==0?TEXT("LOAD SEALED FILM INTO TANK"):Before==1?TEXT("TRANSFER CONTACT PRINT TO FIXER"):TEXT("COLLECT AND EXAMINE PRINT");
  TArray<TSharedRef<SButton>> Buttons;const auto Modal=M->Hud?M->Hud->ModalWidget():nullptr;if(Modal)OrbButtons(Modal.ToSharedRef(),Label,Buttons);
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("native orb development action available: ")+Label))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#else
  Finish(false,TEXT("orb lab harness requires development build"));return;
#endif
  if(!Check(Original()&&Original()->LabStage==Before+1&&F.Budget()==PhotoBudget,TEXT("physical lab callback advances exactly one stage without payment")))return;
  Go(9504,.2);return;
 }
 if(Stage==9506)
 {
  if(JobIndex==1)
  {
   bool Ready=false;if(!Check(F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("OPS"))&&!Ready,TEXT("developed matching control points back to OPS without declaring camera readiness")))return;
   F.Use(TEXT("opsTerminal"));Go(9604,.8);return;
  }
  S47J::Set(Snapshot,TEXT("noiseLoggerBefore"),F.Field->FieldLogger().Save());S47J::Set(Snapshot,TEXT("noiseCasesBefore"),F.CaseFile().Save());
  S47J::Set(Snapshot,TEXT("noiseAlbumBefore"),M->Night->Album->SaveData());S47J::Set(Snapshot,TEXT("noiseDebtBefore"),F.Debt());S47J::Set(Snapshot,TEXT("noiseFocusBefore"),F.Director->PhotoControlFocusId());
  const auto RealAlbum=S47J::Clone(M->Night->Album->SaveData());auto Unavailable=S47J::Clone(RealAlbum);
  for(const auto& V:S47J::Arr(Unavailable,TEXT("shots")))if(S47J::Str(V->AsObject(),TEXT("id"))==RetainedPhotoId)S47J::Set(V->AsObject(),TEXT("unavailable"),true);
  M->Night->Album->Restore(Unavailable);const bool Rejected=!F.Director->PlanLocalNoiseCheck(PhotoClaimId);M->Night->Album->Restore(RealAlbum);
  if(!Check(Rejected,TEXT("optional noise idea rejects unavailable original without altering it")))return;
  if(!Check(F.Use(TEXT("opsTerminal")),TEXT("optional noise idea opens actual OPS")))return;Go(95060,.5);return;
 }
 if(Stage==95060)
 {
  if(!F.TerminalWindowRendered())return;const auto Buttons=NativeButtons(TEXT("SUPPLIES"));
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("native supplies opens optional noise idea")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#endif
  Go(95061,.5);return;
 }
 if(Stage==95061)
 {
  if(!F.TerminalWindowRendered())return;const auto Buttons=NativeButtons(TEXT("Keep a local-noise check for later / no payment"));
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("one optional native plan button for developed real original")))return;
  TSharedPtr<SWidget> W=Buttons[0];while(W.IsValid()){if(W->GetTypeAsString()==TEXT("SScrollBox")){StaticCastSharedPtr<SScrollBox>(W)->ScrollDescendantIntoView(Buttons[0],false);break;}W=W->GetParentWidget();}
  Go(95062,.5);return;
 }
 if(Stage==95062)
 {
  const auto Buttons=NativeButtons(TEXT("Keep a local-noise check for later / no payment"));
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("native optional plan button ready after layout")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#endif
  Go(95063,.4);return;
 }
 if(Stage==95063)
 {
  const auto& Noise=F.Director->PhotoClaims()[0];
  if(!Check(Noise.LocalNoiseOriginalId==RetainedPhotoId&&Noise.LocalNoisePlannedAt>=0&&F.HasFinding(TEXT("photo-local-noise-plan.")+PhotoClaimId)&&!F.Director->PlanLocalNoiseCheck(PhotoClaimId)&&F.Budget()==PhotoBudget&&F.Debt()==S47J::Int(Snapshot,TEXT("noiseDebtBefore"))&&F.Director->PhotoControlFocusId()==S47J::Str(Snapshot,TEXT("noiseFocusBefore"))&&FJsonValue::CompareEqual(FJsonValueObject(F.Field->FieldLogger().Save()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("noiseLoggerBefore"))))&&FJsonValue::CompareEqual(FJsonValueObject(F.CaseFile().Save()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("noiseCasesBefore"))))&&FJsonValue::CompareEqual(FJsonValueObject(M->Night->Album->SaveData()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("noiseAlbumBefore")))),TEXT("native idea retains frozen original once; cash, access, cases, logger tapes/overlap and photographs unchanged")))return;
  S47J::Set(Snapshot,TEXT("noisePlannedAt"),Noise.LocalNoisePlannedAt);
  if(!Check(F.Field->LoggerAccess()||F.Director->LocalNoisePlanHint(PhotoClaimId).Contains(TEXT("THE REPEATED INTERVAL")),TEXT("without logger access the saved idea explains the existing loan gate")))return;
  if(!F.TerminalWindowRendered())return;
  const auto Hints=NoiseHints();if(!Check(Hints.Num()==1,TEXT("exactly one local-noise hint exists in current native UI roots")))return;
  const auto Scroll=HintScroll(Hints[0]);if(!Check(Scroll.IsValid(),TEXT("local-noise hint has actual scroll ancestor")))return;
  UE_LOG(LogTemp,Display,TEXT("S47 noise hint scroll before=%.2f end=%.2f hintHeight=%.2f viewportHeight=%.2f"),Scroll->GetScrollOffset(),Scroll->GetScrollOffsetOfEnd(),Hints[0]->GetCachedGeometry().GetLocalSize().Y,Scroll->GetCachedGeometry().GetLocalSize().Y);
  Scroll->SetScrollOffset(Scroll->GetScrollOffsetOfEnd());
  Go(950631,.5);return;
 }
 if(Stage==950631)
 {
  const auto Hints=NoiseHints();if(!Check(Hints.Num()==1,TEXT("same unique hint remains after scroll layout")))return;
  const auto Scroll=HintScroll(Hints[0]);if(!Check(Scroll.IsValid(),TEXT("scroll ancestor remains after layout")))return;
  // Offscreen AutoWrap text acquires its final height only after first becoming
  // visible. Read the new content extent after that paint, then inspect bounds.
  Scroll->SetScrollOffset(Scroll->GetScrollOffsetOfEnd());Go(950632,.5);return;
 }
 if(Stage==950632)
 {
  const auto Hints=NoiseHints();if(!Check(Hints.Num()==1,TEXT("unique local-noise hint survives final wrap layout")))return;
  const auto Scroll=HintScroll(Hints[0]);if(!Check(Scroll.IsValid(),TEXT("actual scroll viewport retained after text wrapping")))return;
  const auto HG=Hints[0]->GetCachedGeometry(),SG=Scroll->GetCachedGeometry();
  const double Top=HG.LocalToAbsolute(FVector2D::ZeroVector).Y,Bottom=HG.LocalToAbsolute(FVector2D(0,HG.GetLocalSize().Y)).Y;
  const double ViewTop=SG.LocalToAbsolute(FVector2D::ZeroVector).Y,ViewBottom=SG.LocalToAbsolute(FVector2D(0,SG.GetLocalSize().Y)).Y;
  UE_LOG(LogTemp,Display,TEXT("S47 noise hint scroll after=%.2f end=%.2f bounds=%.2f..%.2f viewport=%.2f..%.2f"),Scroll->GetScrollOffset(),Scroll->GetScrollOffsetOfEnd(),Top,Bottom,ViewTop,ViewBottom);
  if(!Check(Bottom>Top&&Top>=ViewTop-2&&Bottom<=ViewBottom+2,TEXT("complete local-noise instruction is geometrically inside native scroll viewport before screenshot")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("orb-optional-local-noise-plan.png"),false,false);Go(95064,.6);return;
 }
 if(Stage==95064)
 {
  M->UI->Close(true);
  if(!Check(F.Director->PlanPhotoControl(PhotoClaimId)&&F.HasFinding(TEXT("orb-return-plan.")+PhotoClaimId)&&F.Budget()==PhotoBudget,TEXT("developed original unlocks persisted S-03 reference plan without payment or manufactured control result")))return;
  if(!Check(!F.Director->PlanPhotoControl(PhotoClaimId)&&F.Director->RegisterPhotoClaim(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("orb plan is idempotent and documentation pays once only")))return;
  if(!Check(F.Director->FollowPhotoControl(PhotoClaimId)&&F.Director->PhotoControlFocusId()==PhotoClaimId&&!F.Director->FollowPhotoControl(TEXT("missing-plan"))&&F.Director->PhotoControlFocusId()==PhotoClaimId,TEXT("real developed plan can be followed at powered OPS; invalid focus leaves it unchanged")))return;
  if(!Restore(TEXT("orb-paid-plan")))return;Go(9507,.5);return;
 }
 if(Stage==9507)
 {
  Ops();Go(9508,.3);return;
 }
 if(Stage==9508)
 {
  const auto& Noise=F.Director->PhotoClaims()[0];
  if(!Check(Noise.LocalNoiseOriginalId==RetainedPhotoId&&Noise.LocalNoisePlannedAt==S47J::Num(Snapshot,TEXT("noisePlannedAt"))&&F.HasFinding(TEXT("photo-local-noise-plan.")+PhotoClaimId)&&!F.Director->PlanLocalNoiseCheck(PhotoClaimId)&&FJsonValue::CompareEqual(FJsonValueObject(F.Field->FieldLogger().Save()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("noiseLoggerBefore"))))&&FJsonValue::CompareEqual(FJsonValueObject(F.CaseFile().Save()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("noiseCasesBefore")))),TEXT("existing disk Continue retains optional noise idea without loan, tape, overlap, access or case changes")))return;
  if(!Check(F.Director->PhotoControlFocusId()==PhotoClaimId,TEXT("disk Continue preserves selected real photo return plan")))return;
  if(!Check(Original()&&Original()->IsDeveloped()&&Original()->Url==RetainedPhotoUrl&&Original()->Clock==S47J::Num(Snapshot,TEXT("photoClock"))&&F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].bPaid&&F.HasFinding(TEXT("orb-return-plan.")+PhotoClaimId)&&!F.Director->PlanPhotoControl(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("disk Continue preserves real developed original, paid incident and unique uncompleted control plan")))return;
  if(!Check(Original()->Pose.IsSet(),TEXT("real orb exposure retained its shutter pose")))return;
  S47J::Set(Snapshot,TEXT("baselineId"),RetainedPhotoId);S47J::Set(Snapshot,TEXT("baselineUrl"),RetainedPhotoUrl);
  const auto Pose=Original()->Pose.GetValue();
  // End this explicitly forced passage; this does not claim natural waiting.
  F.Director->CancelForTest();M->UI->Close(true);M->Player->Place(Pose.Eye.X,Pose.Eye.Z,0);
  M->Player->Yaw=FMath::Atan2(-Pose.Forward.X,-Pose.Forward.Z);M->Player->Pitch=FMath::Atan2(Pose.Forward.Y,FVector2D(Pose.Forward.X,Pose.Forward.Z).Size());
  PhotoBudget=F.Budget();Go(9599,.4);return;
 }
 if(Stage==9599)
 {
  const auto Pose=Original()->Pose.GetValue();
  M->Player->Place(Pose.Eye.X+3,Pose.Eye.Z,0);Go(95991,.3);return;
 }
 if(Stage==95991)
 {
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();bool Ready=false;
  if(!Check(F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("CONTROL POSITION"))&&!Ready,TEXT("live camera hint reports wrong position and does not claim readiness")))return;
  const auto Pose=Original()->Pose.GetValue();M->Player->Place(Pose.Eye.X,Pose.Eye.Z,0);
  M->Player->Yaw=FMath::Atan2(-Pose.Forward.X,-Pose.Forward.Z)+.3;Go(95992,.3);return;
 }
 if(Stage==95992)
 {
  M->FieldCamera->Sync();bool Ready=false;
  if(!Check(F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("MATCH VIEW"))&&!Ready,TEXT("live control hint explains unmatched direction from original position")))return;
  const auto Pose=Original()->Pose.GetValue();M->Player->Yaw=FMath::Atan2(-Pose.Forward.X,-Pose.Forward.Z);M->Player->Pitch=FMath::Atan2(Pose.Forward.Y,FVector2D(Pose.Forward.X,Pose.Forward.Z).Size());Go(9600,.3);return;
 }
 if(Stage==9600)
 {
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();bool Ready=false;
  if(!Check(F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("VIEW MATCHED"))&&Ready,TEXT("actual synced shutter pose passes the same match rule as capture")))return;
  const auto Control=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
  if(!Check(Control.Id!=S47J::Str(Snapshot,TEXT("baselineId"))&&!Control.bUnavailable&&!Control.Url.IsEmpty()&&Control.Pose.IsSet()&&Control.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform-control:")+PhotoClaimId;})&&F.Director->PhotoClaims().Num()==1,TEXT("later real shutter from the original viewpoint retains a separate matching control, not another paid claim")))return;
  bool AfterReady=false;if(!Check(F.Director->PhotoControlFieldHint(AfterReady).Contains(TEXT("PHOTO LAB"))&&!AfterReady,TEXT("new actual control directs to development instead of more exposures")))return;
  RetainedPhotoId=Control.Id;RetainedPhotoUrl=Control.Url;S47J::Set(Snapshot,TEXT("photoClock"),Control.Clock);Ops();Go(9601,.3);return;
 }
 if(Stage==9601)
 {
  if(!Check(!F.Director->RegisterPhotoControl(PhotoClaimId,S47J::Str(Snapshot,TEXT("baselineId")))&&!F.Director->RegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&F.Budget()==PhotoBudget,TEXT("OPS rejects reuse of original and an undeveloped control without payment")))return;
  JobIndex=1;M->UI->Close(true);M->Web->SetDoorOpen(TEXT("lab"),false,true);const auto* Bench=M->Inter->Get(TEXT("wetBench"));if(!Check(Bench&&bool(Bench->Object),TEXT("return control uses same physical photo lab")))return;
  const auto P=Bench->Object.WorldPosition();M->Player->Place(P.X,P.Z+1,0);Go(9504,.3);return;
 }
 if(Stage==9604)
 {
  const auto Buttons=NativeButtons(TEXT("SUPPLIES"));
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("native OPS supplies navigation is available for paired originals")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#endif
  Go(96041,.4);return;
 }
 if(Stage==96041)
 {
  if(!Check(F.OperationsSection()==5,TEXT("OPS displays developed photographic comparison in supplies")))return;
  if(!F.TerminalWindowRendered())return;
  const auto Buttons=NativeButtons(TEXT("Retain comparison / no payment"));
  if(!Check(Buttons.Num()==1,TEXT("paired originals have one comparison action to scroll into view")))return;
  TSharedPtr<SWidget> W=Buttons[0];while(W.IsValid())
  {
   if(W->GetTypeAsString()==TEXT("SScrollBox")){StaticCastSharedPtr<SScrollBox>(W)->ScrollDescendantIntoView(Buttons[0],false);break;}
   W=W->GetParentWidget();
  }
  Go(960411,.5);return;
 }
 if(Stage==960411)
 {
  UE_LOG(LogTemp,Display,TEXT("S47 photo pair viewport: FOV=%.3f eye=%s rotation=%s onGlass=%d rendered=%d"),M->Player->Fov,*M->Player->EyeUE().ToString(),*M->Player->GetActorRotation().ToString(),F.TerminalOnGlass(),F.TerminalWindowRendered());
  FVector Centre,Up,Across;FVector2D Size;M->Web->CrtGlass(3,Centre,Up,Across,Size);
  UE_LOG(LogTemp,Display,TEXT("S47 photo pair CRT centre=%s across=%s up=%s size=%s"),*Centre.ToString(),*Across.ToString(),*Up.ToString(),*Size.ToString());
  for(int32 I=0;I<M->Web->PartCount();++I)if(const auto* N=M->Web->Node(I))
  {
   auto* C=Cast<UMeshComponent>(N->Comp.Get());if(!C||FVector::Dist(C->Bounds.Origin,Centre)>3)continue;
   for(int32 J=0;J<C->GetNumMaterials();++J)if(auto* Mat=C->GetMaterial(J))
   {
    UTexture* Texture=nullptr;Mat->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")),Texture);
    if(auto* RT=Cast<UTextureRenderTarget2D>(Texture))
    {
     TArray<FColor> Pixels;if(!S47Image::ReadPixels(RT,Pixels))continue;
     FString Url=S47Image::ToDataUrl(Pixels,RT->SizeX,RT->SizeY,false);int32 Comma=0;TArray<uint8> Bytes;
     if(Url.FindChar(',',Comma)&&FBase64::Decode(Url.Mid(Comma+1),Bytes))FFileHelper::SaveArrayToFile(Bytes,*(Root/TEXT("orb-comparison-native-crt.png")));
     UE_LOG(LogTemp,Display,TEXT("S47 photo pair native target node=%s size=%dx%d"),*N->Name,RT->SizeX,RT->SizeY);
    }
   }
  }
  // Capture the CRT world texture through viewport readback, independent of Slate composition.
  FScreenshotRequest::RequestScreenshot(Root/TEXT("orb-two-developed-originals.png"),false,false);Go(96042,.4);return;
 }
 if(Stage==96042)
 {
  const auto Buttons=NativeButtons(TEXT("Retain comparison / no payment"));
  if(!Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("both originals have an enabled native comparison button")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#endif
  if(!Check(F.Director->PhotoClaims()[0].ControlPhotoId==RetainedPhotoId&&F.HasFinding(TEXT("orb-photo-comparison.")+PhotoClaimId)&&!F.Director->RegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&F.Budget()==PhotoBudget,TEXT("native comparison click retains the pair once without payment; subsequent API repeat is rejected")))return;
  Go(9605,.4);return;
 }
 if(Stage==9605)
 {
  M->UI->Close(true);if(!Restore(TEXT("orb-control-compared")))return;Go(9606,.5);return;
 }
 if(Stage==9606)
 {
  Ops();Go(9607,.3);return;
 }
 if(Stage==9607)
 {
  const auto* Baseline=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==S47J::Str(Snapshot,TEXT("baselineId"));});
  const auto& Claim=F.Director->PhotoClaims()[0];
  if(!Check(Baseline&&Baseline->Url==S47J::Str(Snapshot,TEXT("baselineUrl"))&&Original()&&Original()->Url==RetainedPhotoUrl&&Baseline->Pose.IsSet()&&Original()->Pose.IsSet()&&Baseline->Pose->Matches(Original()->Pose.GetValue())&&Claim.ControlOriginalId==Baseline->Id&&Claim.ControlPhotoId==Original()->Id&&F.HasFinding(TEXT("orb-photo-comparison.")+PhotoClaimId)&&!F.Director->RegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&F.Budget()==PhotoBudget,TEXT("Continue retains both unchanged developed originals, frozen baseline and comparison without second reward")))return;
  // Availability fixtures follow the real journey; never count fabricated photographs as capture evidence.
  const auto SavedAlbum=S47J::Read(S47J::Write(M->Night->Album->SaveData()));const auto SavedDirector=S47J::Read(S47J::Write(F.Director->Save()));
  auto Plans=S47J::Read(S47J::Write(SavedDirector));auto Entries=S47J::Arr(Plans,TEXT("photoClaims"));
  if(!Check(Entries.Num()==1,TEXT("availability fixture starts with one real retained incident")))return;
  auto Missing=S47J::Read(S47J::Write(Entries[0]->AsObject()));Missing->RemoveField(TEXT("controlPhoto"));S47J::Set(Missing,TEXT("controlOriginal"),TEXT("missing-original-fixture"));
  auto Valid=S47J::Read(S47J::Write(Entries[0]->AsObject()));Valid->RemoveField(TEXT("controlPhoto"));S47J::Set(Valid,TEXT("id"),TEXT("dusk_orb.availability-fixture"));
  Plans->SetArrayField(TEXT("photoClaims"),{MakeShared<FJsonValueObject>(Missing),MakeShared<FJsonValueObject>(Valid)});F.Director->Begin(Plans);
  bool Ready=false;
  if(!Check(!F.Director->CanRegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&!F.Director->RegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&F.Budget()==PhotoBudget,TEXT("missing frozen original disables the UI predicate and backend without payment")))return;
  const FString Next=F.Director->PhotoControlFieldHint(Ready);
  if(!Check(!Next.IsEmpty()&&!Next.Contains(TEXT("ORIGINAL VIEW UNAVAILABLE")),TEXT("later usable pending plan receives field guidance despite first missing original")))return;
  // Serialized round trip of the two plans: this fixture does not claim a further disk Continue.
  F.Director->Begin(S47J::Read(S47J::Write(F.Director->Save())));Ready=false;
  if(!Check(F.Director->PhotoClaims().Num()==2&&!F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("ORIGINAL VIEW UNAVAILABLE")),TEXT("both pending plans and usable-plan selection survive serialization")))return;
  Plans->SetArrayField(TEXT("photoClaims"),{MakeShared<FJsonValueObject>(Missing)});F.Director->Begin(Plans);Ready=true;
  if(!Check(F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("ORIGINAL VIEW UNAVAILABLE"))&&!Ready,TEXT("only missing original reports unavailability rather than a shutter instruction")))return;
  auto Unavailable=S47J::Read(S47J::Write(SavedAlbum));auto Shots=S47J::Arr(Unavailable,TEXT("shots"));
  for(const auto& V:Shots)if(S47J::Str(V->AsObject(),TEXT("id"))==Baseline->Id){S47J::Set(V->AsObject(),TEXT("unavailable"),true);S47J::Set(V->AsObject(),TEXT("labStage"),0);}
  auto Pending=S47J::Read(S47J::Write(SavedDirector));auto PendingClaims=S47J::Arr(Pending,TEXT("photoClaims"));PendingClaims[0]->AsObject()->RemoveField(TEXT("controlPhoto"));F.Director->Begin(Pending);
  if(!Check(F.Director->CanRegisterPhotoControl(PhotoClaimId,RetainedPhotoId),TEXT("restored real developed pair remains eligible before missing-original fixture")))return;
  M->Night->Album->Restore(Unavailable);
  if(!Check(F.Director->LocalNoisePlanHint(PhotoClaimId).Contains(TEXT("reference photograph is unavailable")),TEXT("missing frozen noise reference is reported honestly while its saved idea remains")))return;
  Ready=true;if(!Check(!F.Director->CanRegisterPhotoControl(PhotoClaimId,RetainedPhotoId)&&F.Director->PhotoControlFieldHint(Ready).Contains(TEXT("ORIGINAL VIEW UNAVAILABLE"))&&!Ready,TEXT("marked unavailable undeveloped original is never treated as a usable control baseline")))return;
  M->Night->Album->Restore(SavedAlbum);F.Director->Begin(SavedDirector);
  if(!Check(F.Budget()==PhotoBudget&&S47J::Write(M->Night->Album->SaveData())==S47J::Write(SavedAlbum)&&F.Director->PhotoClaims()[0].ControlPhotoId==RetainedPhotoId,TEXT("availability fixtures leave actual originals, comparison history and payment unchanged")))return;
  // Two pending UI fixtures share the actual developed baseline. These are
  // NOT two observed incidents, photographs, or new payment opportunities.
  S47J::Set(Snapshot,TEXT("nativeFocusRealDirector"),SavedDirector);
  S47J::Set(Snapshot,TEXT("nativeFocusRealAlbum"),SavedAlbum);
  auto UIPlans=S47J::Clone(SavedDirector);const auto RealClaims=S47J::Arr(UIPlans,TEXT("photoClaims"));
  auto First=S47J::Clone(RealClaims[0]->AsObject()),Second=S47J::Clone(RealClaims[0]->AsObject());
  First->RemoveField(TEXT("controlPhoto"));Second->RemoveField(TEXT("controlPhoto"));
  S47J::Set(First,TEXT("id"),TEXT("dusk_orb.native-focus-first-fixture"));
  S47J::Set(Second,TEXT("id"),TEXT("dusk_orb.native-focus-second-fixture"));
  S47J::Set(First,TEXT("title"),TEXT("UI FIXTURE A / same retained original"));
  S47J::Set(Second,TEXT("title"),TEXT("UI FIXTURE B / same retained original"));
  // Regression fixture: a newer best incident image may be undeveloped while
  // the plan still owns the earlier developed baseline. Reuse retained image
  // data only as fixture input; this does not claim another natural exposure.
  S47J::Set(Second,TEXT("photo"),RetainedPhotoId);S47J::Set(Second,TEXT("paid"),false);
  auto UndevelopedAlbum=S47J::Clone(SavedAlbum);
  for(const auto& V:S47J::Arr(UndevelopedAlbum,TEXT("shots")))if(S47J::Str(V->AsObject(),TEXT("id"))==RetainedPhotoId)S47J::Set(V->AsObject(),TEXT("labStage"),0);
  M->Night->Album->Restore(UndevelopedAlbum);
  UIPlans->SetArrayField(TEXT("photoClaims"),{MakeShared<FJsonValueObject>(First),MakeShared<FJsonValueObject>(Second)});
  S47J::Set(UIPlans,TEXT("photoControlFocus"),TEXT("dusk_orb.native-focus-first-fixture"));
  F.Director->Begin(UIPlans);S47J::Set(Snapshot,TEXT("nativeFocusFixtureBefore"),F.Director->Save());
  M->UI->Close(true);Ops();Go(9610,.4);return;
 }

 if(Stage==9610)
 {
  if(!FocusCheck(F.Director->PhotoClaims().Num()==2&&F.Director->PhotoControlFocusId()==TEXT("dusk_orb.native-focus-first-fixture")&&F.Budget()==PhotoBudget,TEXT("explicit two-pending-plan UI fixture uses one actual original and unchanged cash")))return;
  if(!FocusCheck(F.Use(TEXT("opsTerminal")),TEXT("fixture opens actual powered OPS terminal")))return;
  Go(9611,.5);return;
 }
 if(Stage==9611)
 {
  if(!F.TerminalWindowRendered())return;
  const auto Buttons=NativeButtons(TEXT("SUPPLIES"));
  if(!FocusCheck(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("native supplies navigation available for focus fixture")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#else
  FocusCheck(false,TEXT("native focus fixture requires Development build"));return;
#endif
  Go(96120,.5);return;
 }
 if(Stage==96120)
 {
  if(!F.TerminalWindowRendered())return;
  const auto Follow=NativeButtons(TEXT("Follow this return plan"));
  if(!FocusCheck(Original()&&!Original()->IsDeveloped()&&Follow.Num()==1&&Follow[0]->IsEnabled()&&NativeButtons(TEXT("Register photograph / $20 documentation credit")).IsEmpty(),TEXT("newer undeveloped claim-photo fixture leaves frozen plan Follow available, without payment action")))return;
  // Second case: current claim image is unavailable, frozen original is intact.
  M->Night->Album->Restore(S47J::Clone(S47J::Obj(Snapshot,TEXT("nativeFocusRealAlbum"))));
  auto MissingClaim=F.Director->Save();auto Plans=S47J::Arr(MissingClaim,TEXT("photoClaims"));
  S47J::Set(Plans[1]->AsObject(),TEXT("photo"),TEXT("missing-current-claim-photo-fixture"));
  F.Director->Begin(MissingClaim);S47J::Set(Snapshot,TEXT("nativeFocusFixtureBefore"),F.Director->Save());
  F.OpenOperations();Go(9612,.5);return;
 }
 if(Stage==9612)
 {
  if(!F.TerminalWindowRendered())return;
  const auto Buttons=NativeButtons(TEXT("Follow this return plan"));
  if(!FocusCheck(F.OperationsSection()==5&&Buttons.Num()==1&&Buttons[0]->IsEnabled()&&NativeButtons(TEXT("Register photograph / $20 documentation credit")).IsEmpty(),TEXT("unavailable current claim image does not hide second frozen-plan Follow or offer payment")))return;
  TSharedPtr<SWidget> W=Buttons[0];while(W.IsValid())
  {
   if(W->GetTypeAsString()==TEXT("SScrollBox")){StaticCastSharedPtr<SScrollBox>(W)->ScrollDescendantIntoView(Buttons[0],false);break;}
   W=W->GetParentWidget();
  }
  Go(9613,.5);return;
 }
 if(Stage==9613)
 {
  const auto Buttons=NativeButtons(TEXT("Follow this return plan"));
  if(!FocusCheck(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("second plan native Follow button remains available after Slate layout")))return;
#if !UE_BUILD_SHIPPING
  Buttons[0]->SimulateClick();
#endif
  Go(9614,.4);return;
 }
 if(Stage==9614)
 {
  const auto Before=S47J::Obj(Snapshot,TEXT("nativeFocusFixtureBefore")),After=F.Director->Save();
  const auto BeforeClaims=MakeShared<FJsonValueArray>(S47J::Arr(Before,TEXT("photoClaims")));
  const auto AfterClaims=MakeShared<FJsonValueArray>(S47J::Arr(After,TEXT("photoClaims")));
  if(!FocusCheck(F.Director->PhotoControlFocusId()==TEXT("dusk_orb.native-focus-second-fixture")&&FJsonValue::CompareEqual(*BeforeClaims,*AfterClaims)&&F.Budget()==PhotoBudget&&FJsonValue::CompareEqual(FJsonValueObject(M->Night->Album->SaveData()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("nativeFocusRealAlbum")))),TEXT("native Follow click selects second ID; both fixture plans, real originals and cash remain unchanged")))return;
  RestoreFocusFixture();
  const auto Real=S47J::Obj(Snapshot,TEXT("nativeFocusRealDirector"));
  if(!Check(FJsonValue::CompareEqual(FJsonValueObject(F.Director->Save()),FJsonValueObject(Real))&&F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].Id==PhotoClaimId&&F.Director->PhotoClaims()[0].ControlPhotoId==RetainedPhotoId&&F.Budget()==PhotoBudget,TEXT("after UI fixture, exact genuine Director history and completed paid original pair restored")))return;
  Snapshot->RemoveField(TEXT("nativeFocusRealDirector"));Snapshot->RemoveField(TEXT("nativeFocusRealAlbum"));Snapshot->RemoveField(TEXT("nativeFocusFixtureBefore"));
  if(FParse::Param(FCommandLine::Get(),TEXT("S47NotebookFocusTest"))){Go(9700,.4);return;}
  Go(9800,.2);return;
 }
 // Forced second appearance, only to walk up to it: the lights must leave, straight up, without any record.
 if(Stage==9800)
 {
  M->UI->Close(true);if(!Check(F.Director->Force(TEXT("dusk_orb")),TEXT("orb lights can appear again for the approach check")))return;
  M->Player->Place(F.Director->EventAnchor().X+30,F.Director->EventAnchor().Z+30,0);Go(9801,4.);return;
 }
 if(Stage==9801)
 {
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb"),TEXT("orb lights stay while nobody is near")))return;
  const FVector P=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());M->Player->Place(P.X,P.Z+3,0);
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  M->UI->ClearToasts();Go(9802,.5);return;
 }
 if(Stage==9802){Shot(TEXT("orb-startled"));Go(9803,2.2);return;}
 if(Stage==9803)
 {
  if(!Check(F.Director->ActiveEvent().IsEmpty(),TEXT("a person coming close sends the lights straight up and ends the appearance")))return;
  Finish(true,TEXT("PHOTO04 real-original journey and optional local-noise idea; explicit synthetic two-plan Follow wiring restores real history. No logger measurement, second natural event or additional payment claimed."));return;
 }

 // The visit in the dark: nothing below forces the event; the clock is only moved to the hour the schedule itself holds.
 if(Stage==9900)
 {
  const double Due=S47J::Num(OrbSchedule(),TEXT("nightDue"),-1);
  if(Due<0){if(Age-At>20){Finish(false,TEXT("no night visit was scheduled after the first shift"));}return;}
  const double Hour=FMath::Fmod(Due,86400.)/3600.;
  if(!Check(Hour>=20&&Hour<24.5&&F.Director->ActiveEvent().IsEmpty()&&F.TestAdvance(FMath::Max(0.,Due-F.Clock()-1.)),TEXT("schedule holds its own hour in the dark; clock moved to one second before it, past the dusk window")))return;
  Go(9901);return;
 }
 if(Stage==9901)
 {
  if(F.Director->ActiveEvent().IsEmpty()){if(Age-At>60){Finish(false,TEXT("the schedule's night visit never started"));}return;}
  const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb")&&Hour>=20&&Hour<24.5,TEXT("the lights start in the dark, from the scheduler and outside the dusk window")))return;
  Go(9902,4.);return;
 }
 if(Stage==9902)
 {
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb")&&F.Director->PhysicalObjectPresented(),TEXT("night visit has a presented native component after gradual appearance")))return;
  int32 On=0,Shadowed=0;float Lumens=0,Radius=0;F.Director->OrbLightCount(On,Shadowed,Lumens,Radius);
  UE_LOG(LogTemp,Display,TEXT("S47 orb lights on=%d shadowed=%d lumens=%.0f radius=%.0f cm"),On,Shadowed,Lumens,Radius);
  if(!Check(On>=1&&On<=FS47DuskOrb::Count&&Shadowed==0&&Lumens>0&&Radius>0,TEXT("one unshadowed point light per orb, within the cap")))return;
  const FVector Home=F.Director->EventAnchor();
  for(const auto& Hover:FS47DuskOrb::Stops())
  {
   const auto* Thing=M->Web->Item(Hover.Item);
   if(!Check(Thing!=nullptr,FString::Printf(TEXT("the world has %s"),Hover.Item)))return;
   const double DX=FMath::Max3(Thing->Min.X-Hover.X,0.,Hover.X-Thing->Max.X),DZ=FMath::Max3(Thing->Min.Z-Hover.Z,0.,Hover.Z-Thing->Max.Z);
   const double Gap=FMath::Sqrt(DX*DX+DZ*DZ),Above=Home.Y+Hover.Up-FS47DuskOrb::HomeUp-US47Night::SaroFloorAt(M->Web.Get(),Hover.X,Hover.Z);
   UE_LOG(LogTemp,Display,TEXT("S47 orb stop %s: %.2f m from %s, %.2f m above the ground"),Hover.Name,Gap,Hover.Item,Above);
   if(!Check(Gap>0&&Gap<4.5&&Above>=.9&&Above<=5.5,FString::Printf(TEXT("%s is beside the world's own %s and above its actual ground"),Hover.Name,Hover.Item)))return;
  }
  S47J::Set(Snapshot,TEXT("age"),F.Director->EventAge());S47J::Set(Snapshot,TEXT("anchor"),TArray<double>{Home.X,Home.Y,Home.Z});
  if(!Restore(TEXT("orb-night-active")))return;Go(9903,.5);return;
 }
 if(Stage==9903)
 {
  const auto A=S47J::Nums(Snapshot,TEXT("anchor"));const double Before=S47J::Num(Snapshot,TEXT("age"));const auto Sched=OrbSchedule();
  if(!Check(A.Num()==3&&F.Director->ActiveEvent()==TEXT("dusk_orb")&&F.Director->EventAnchor().Equals(FVector(A[0],A[1],A[2]),.001)&&F.Director->EventAge()>=Before&&F.Director->EventAge()<Before+3&&S47J::Int(Sched,TEXT("nightVisits"))==1&&S47J::Num(Sched,TEXT("nightDue"))>F.Clock()+3600.,TEXT("Continue retains the night visit's path and its consumed schedule")))return;
  M->UI->Close(true);M->Web->SetDoorOpen(TEXT("lab"),false,true);const auto* Bench=M->Inter->Get(TEXT("wetBench"));
  if(!Check(Bench&&bool(Bench->Object),TEXT("windowless photo lab exists for the sight check")))return;
  const FVector P=Bench->Object.WorldPosition();M->Player->Place(P.X,P.Z+1,0);AimAt(F.Director->OrbRecordAt());M->UI->ClearToasts();Go(9904,.4);return;
 }
 if(Stage==9904)
 {
  // Indoors, aimed straight at where it is: the walls are between, and neither a line of sight nor a record exists.
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(OrbNightWalls),true);Params.AddIgnoredActor(M->Player);
  const FVector Orb=S47Three::ToUE(F.Director->OrbRecordAt());
  const bool bBlocked=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,M->Player->EyeUE(),Orb,ECC_Visibility,Params);
  UE_LOG(LogTemp,Display,TEXT("S47 orb sight from the lab: blocked=%d by %s, %.1f m short of it"),bBlocked,Hit.Component.IsValid()?*Hit.Component->GetName():TEXT("nothing"),bBlocked?FVector::Dist(Hit.Location,Orb)*.01:0.);
  if(!Check(bBlocked&&FVector::Dist(Hit.Location,Orb)>200.,TEXT("a building stands between the lab and the lights")))return;
  if(!Check(M->Night->Album->Left()>=2,TEXT("film for the hidden and the clear exposure")))return;
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Hidden=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
  const bool bTagged=Hidden.Targets.ContainsByPredicate([](const TPair<FString,FVector2D>& Target){return Target.Key.StartsWith(TEXT("freeform:"));});
  if(!Check(!bTagged&&F.Director->PhotoClaims().IsEmpty(),TEXT("an exposure with the building in the way registers nothing")))return;
  Go(9905);return;
 }
 if(Stage==9905)
 {
  const int32 StopIndex=FS47DuskOrb::StopAt(F.Director->EventAge());
  if(StopIndex<0){if(Age-At>30){Finish(false,TEXT("the first orb never reached a piece of equipment"));}return;}
  const FVector P=OrbNow();UE_LOG(LogTemp,Display,TEXT("S47 orb night: first orb holding at %s, %s"),FS47DuskOrb::Stops()[StopIndex].Name,*P.ToString());
  // Only the viewer moves. Nine metres off, looking at the ground under it.
  M->UI->Close(true);M->Player->Place(P.X,P.Z+9,0);AimAt(FVector(P.X,US47Night::SaroFloorAt(M->Web.Get(),P.X,P.Z)+.4,P.Z));M->UI->ClearToasts();Go(9906,.9);return;
 }
 if(Stage==9906){Shot(TEXT("orb-night-at-equipment"));Go(9907,.2);return;}
 if(Stage==9907){F.Director->TestOrbLightProbe(1);Go(9908,.35);return;}
 if(Stage==9908)
 {
  FScreenshotRequest::RequestScreenshot(Root/TEXT("orb-night-probe-lit.png"),false,false);Go(99081,.7);return;
 }
 if(Stage==99081){F.Director->TestOrbLightProbe(2);Go(99082,.35);return;}
 if(Stage==99082){FScreenshotRequest::RequestScreenshot(Root/TEXT("orb-night-probe-unlit.png"),false,false);Go(9909,.7);return;}
 if(Stage==9909)
 {
  const double LitMean=PictureMean(Root/TEXT("orb-night-probe-lit.png")),DarkMean=PictureMean(Root/TEXT("orb-night-probe-unlit.png"));F.Director->TestOrbLightProbe(0);
  UE_LOG(LogTemp,Display,TEXT("S47 orb light probe: lit=%.2f unlit=%.2f of 255"),LitMean,DarkMean);
  if(!Check(LitMean>=0&&DarkMean>=0&&LitMean-DarkMean>=.5,TEXT("with the effect hidden, the orbs' lights alone brighten the ground and equipment in the picture")))return;
  AimAt(OrbNow());Go(9910,.12);return;
 }
 if(Stage==9910)
 {
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
  if(!Check(!Photo.bUnavailable&&!Photo.Url.IsEmpty()&&F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].Id.StartsWith(TEXT("dusk_orb."))&&F.Budget()==PhotoBudget&&Photo.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& Target){return Target.Key==TEXT("freeform:")+F.Director->PhotoClaims()[0].Id;}),TEXT("actual exposure of the night visit qualifies one unpaid claim, as at dusk")))return;
  PhotoClaimId=F.Director->PhotoClaims()[0].Id;RetainedPhotoId=Photo.Id;RetainedPhotoUrl=Photo.Url;
  if(!Restore(TEXT("orb-night-photo")))return;Go(9911,.5);return;
 }
 if(Stage==9911)
 {
  const auto Sched=OrbSchedule();
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb")&&F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].Id==PhotoClaimId&&Original()&&Original()->Url==RetainedPhotoUrl&&S47J::Bool(Sched,TEXT("photographed"))&&S47J::Int(Sched,TEXT("nightVisits"))==1,TEXT("Continue retains the night photograph, its one claim and the record")))return;
  M->Player->Place(-.9,-3.55,0);AimAt(OrbNow());M->UI->ClearToasts();Go(9912,1.8);return;
 }
 if(Stage==9912){Shot(TEXT("orb-night-control-room-window"));Go(9913,.3);return;}
 if(Stage==9913)
 {
  const FVector P=OrbNow();M->Player->Place(P.X,P.Z+3,0);AimAt(P);M->UI->ClearToasts();Go(9914,.7);return;
 }
 if(Stage==9914)
 {
  Shot(TEXT("orb-night-startled"));
  const FVector Seen=F.Director->OrbRecordAt(),Route=OrbNow();
  UE_LOG(LogTemp,Display,TEXT("S47 orb night flight: %.1f m above its route, %.1f m aside"),Seen.Y-Route.Y,FVector2D::Distance(FVector2D(Seen.X,Seen.Z),FVector2D(Route.X,Route.Z)));
  if(!Check(F.Director->ActiveEvent()==TEXT("dusk_orb")&&Seen.Y-Route.Y>15&&FVector2D::Distance(FVector2D(Seen.X,Seen.Z),FVector2D(Route.X,Route.Z))<3,TEXT("a person coming close sends the first light straight up from where it was")))return;
  Go(9915,1.6);return;
 }
 if(Stage==9915)
 {
  if(!Check(F.Director->ActiveEvent().IsEmpty()&&F.Budget()==PhotoBudget,TEXT("the night visit ends when they have gone, with nothing paid")))return;
  Finish(true,TEXT("Night visit from the scheduler's own hour, no Force; clock and player placement are declared fixtures. Building sight, light on the world, photograph, two Continues and flight checked. Whether it looks right is Tom's to judge."));return;
 }

 if(Stage==9700)
 {
  S47J::Set(Snapshot,TEXT("notebookRealSession"),F.Save());S47J::Set(Snapshot,TEXT("notebookRealAlbum"),M->Night->Album->SaveData());
  auto D=F.Director->Save();const auto Real=S47J::Arr(D,TEXT("photoClaims"));
  if(!NotebookCheck(Real.Num()==1,TEXT("notebook fixture starts from actual completed single paid orb journey")))return;
  auto A=S47J::Clone(Real[0]->AsObject()),B=S47J::Clone(A);
  for(auto O:{A,B}){O->RemoveField(TEXT("controlPhoto"));S47J::Set(O,TEXT("paid"),true);}
  S47J::Set(A,TEXT("id"),TEXT("dusk_orb.notebook-a"));S47J::Set(A,TEXT("title"),TEXT("NOTEBOOK FIXTURE A"));
  S47J::Set(B,TEXT("id"),TEXT("dusk_orb.notebook-b"));S47J::Set(B,TEXT("title"),TEXT("NOTEBOOK FIXTURE B"));
  D->SetArrayField(TEXT("photoClaims"),{MakeShared<FJsonValueObject>(A),MakeShared<FJsonValueObject>(B)});S47J::Set(D,TEXT("photoControlFocus"),TEXT("dusk_orb.notebook-a"));F.Director->Begin(D);
  M->UI->Close(true);Ops();Go(9701,.4);return;
 }
 if(Stage==9701)
 {
  M->UI->NotebookAt(TEXT("tasks"));M->Flow->OpenNotebook();Go(9702,.5);return;
 }
 if(Stage==9702)
 {
  const FString T=NotebookText();
  if(!NotebookCheck(T.Contains(TEXT("Return photograph / NOTEBOOK FIXTURE A"))&&T.Contains(TEXT("Return photograph / NOTEBOOK FIXTURE B"))&&T.Contains(TEXT("Camera return guide / selected"))&&!T.Contains(TEXT("Photo documentation / NOTEBOOK")),TEXT("actual rendered Now includes both explicitly paid pending plans, not new payment tasks")))return;
  if(!NotebookCheck(NotebookClick(TEXT("Return photograph / NOTEBOOK FIXTURE B")),TEXT("actual notebook SButton entry click selects second plan")))return;Go(9703,.4);return;
 }
 if(Stage==9703)
 {
  if(!NotebookCheck(NotebookText().Contains(TEXT("RETURN / From S-03"))&&NotebookText().Contains(TEXT("WHERE / SARO / original viewpoint")),TEXT("actual second-plan page renders retained viewpoint directions and location")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("notebook-paid-return-plan.png"),true,false);Go(9704,.4);return;
 }
 if(Stage==9704)
 {
  if(!NotebookCheck(NotebookClick(TEXT("FOLLOW")),TEXT("actual notebook FOLLOW action calls existing Director focus API at OPS")))return;Go(9705,.4);return;
 }
 if(Stage==9705)
 {
  if(!NotebookCheck(F.Director->PhotoControlFocusId()==TEXT("dusk_orb.notebook-b")&&F.Budget()==PhotoBudget&&FJsonValue::CompareEqual(FJsonValueObject(M->Night->Album->SaveData()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("notebookRealAlbum")))),TEXT("native Now Follow selects second fixture without payment or photograph changes")))return;
  M->UI->Close(true);auto D=F.Director->Save();auto C=S47J::Arr(D,TEXT("photoClaims"));S47J::Set(C[1]->AsObject(),TEXT("controlOriginal"),TEXT("missing-notebook-reference"));F.Director->Begin(D);
  if(!NotebookCheck(F.Director->PhotoControlFocusId()==TEXT("dusk_orb.notebook-a"),TEXT("missing selected frozen original falls back to surviving first guide")))return;
  M->Flow->OpenNotebook();Go(9706,.4);return;
 }
 if(Stage==9706)
 {
  if(!NotebookCheck(NotebookClick(TEXT("Return photograph / NOTEBOOK FIXTURE B")),TEXT("missing frozen-reference plan remains selectable in Now")))return;Go(9707,.4);return;
 }
 if(Stage==9707)
 {
  const FString T=NotebookText();
  if(!NotebookCheck(T.Contains(TEXT("Frozen original unavailable"))&&T.Contains(TEXT("Reference unavailable / plan retained"))&&NotebookButtons(TEXT("FOLLOW")).IsEmpty(),TEXT("actual missing-reference detail states unavailable and offers no rejected Follow action")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("notebook-missing-reference.png"),true,false);Go(9708,.4);return;
 }
 if(Stage==9708)
 {
  M->UI->Close(true);
  if(!NotebookCheck(F.Field->TriggerServiceFault(false,TEXT("S-07"))&&F.StartServiceGuide(false)&&F.Field->ServiceDishId()==TEXT("S-07"),TEXT("explicit real S-07 service-fault fixture and production work-order acceptance; no simulated completed repair")))return;
  M->Flow->OpenNotebook();Go(9709,.5);return;
 }
 if(Stage==9709)
 {
  const FString T=NotebookText();const int32 Following=NotebookFollowing();
  if(!NotebookCheck(T.Contains(TEXT("S-07 service work order"))&&!T.Contains(TEXT("S-03 service work order"))&&Following==1,TEXT("actual Now list has exactly one Following label for real non-S03 service order")))return;
  if(!NotebookCheck(NotebookClick(TEXT("S-07 service work order")),TEXT("actual service entry opens its current work-order detail")))return;Go(9710,.4);return;
 }
 if(Stage==9710)
 {
  const FString T=NotebookText();if(!NotebookCheck(T.Contains(TEXT("S-07 AZ / EL DRIVE"))&&T.Contains(TEXT("local service box / S-07")),TEXT("service Now detail agrees on actual named antenna in next action and location")))return;
  FScreenshotRequest::RequestScreenshot(Root/TEXT("notebook-s07-service-focus.png"),true,false);Go(9711,.4);return;
 }
 if(Stage==9711)
 {
  RestoreNotebook();
  if(!Check(F.Budget()==PhotoBudget&&FJsonValue::CompareEqual(FJsonValueObject(F.Director->Save()),FJsonValueObject(S47J::Obj(S47J::Obj(Snapshot,TEXT("notebookRealSession")),TEXT("director"))))&&FJsonValue::CompareEqual(FJsonValueObject(M->Night->Album->SaveData()),FJsonValueObject(S47J::Obj(Snapshot,TEXT("notebookRealAlbum"))))&&F.Field->ServiceStage()==S47J::Int(S47J::Obj(S47J::Obj(Snapshot,TEXT("notebookRealSession")),TEXT("field")),TEXT("service")),TEXT("notebook fixtures restore genuine Director/history, originals, economy and prior service stage")))return;
  Finish(true,TEXT("PHOTO04 plus actual Now/SButton callbacks: explicit paid two-plan and missing-reference fixtures, real S-07 fault/acceptance fixture; all genuine history restored. Not two natural incidents, OS clicking, or completed service."));return;
 }

}
