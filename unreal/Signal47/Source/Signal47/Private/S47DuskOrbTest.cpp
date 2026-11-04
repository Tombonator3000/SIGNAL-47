#include "S47FreeformTest.h"
#include "S47DuskOrb.h"
#include "S47Freeform.h"
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
#include "Materials/MaterialInterface.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
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
 if(Age>150){Finish(false,TEXT("bounded dusk-orb native test timeout"));return;}
 auto Original=[&]()->const FS47AlbumShot*{return M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});};
 auto Ops=[&]{const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X+1.,P.Z,PI/2);}; // OPS is on the west wall, facing +X into the room.
 if(Stage==0)
 {
  if(Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->FieldCamera&&M->Night&&M->Night->Album&&F.Director,TEXT("orb harness requires isolated development profile and native photographic systems")))return;
  M->UI->Close(true);
  if(!M->FieldCamera->bHave){M->Player->Place(17.25,-1.9,0);M->Inter->UseId(TEXT("fieldCamera"));}
  if(!Check(M->FieldCamera->bHave,TEXT("orb fixture collects camera through physical lab callback")))return;
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
  const FVector P=FS47DuskOrb::Position(F.Director->EventAnchor(),F.Director->EventAge());M->Player->Place(P.X,P.Z+6,0);
  const FVector D=P-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());M->UI->ClearToasts();Go(9502,2.3);return;
 }
 if(Stage==9502)
 {
  Shot(TEXT("orb-native-before-exposure"));Go(95021,.4);return;
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
  if(!Check(F.Director->PlanPhotoControl(PhotoClaimId)&&F.HasFinding(TEXT("orb-return-plan.")+PhotoClaimId)&&F.Budget()==PhotoBudget,TEXT("developed original unlocks persisted S-03 reference plan without payment or manufactured control result")))return;
  if(!Check(!F.Director->PlanPhotoControl(PhotoClaimId)&&F.Director->RegisterPhotoClaim(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("orb plan is idempotent and documentation pays once only")))return;
  if(!Restore(TEXT("orb-paid-plan")))return;Go(9507,.5);return;
 }
 if(Stage==9507)
 {
  Ops();Go(9508,.3);return;
 }
 if(Stage==9508)
 {
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
  Finish(true,TEXT("PHOTO04 native paired-original journey: real orb capture, development, saved plan, explicit return pose, separate actual control capture, second physical development and idempotent OPS comparison. No cause inferred; encounter ending and travel are fixtures."));return;
 }

}
