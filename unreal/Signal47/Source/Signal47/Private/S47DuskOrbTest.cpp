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
#include "Widgets/Input/SButton.h"
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
 if(Age>150){Finish(false,TEXT("bounded dusk-orb native test timeout"));return;}
 auto Original=[&]()->const FS47AlbumShot*{return M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& P){return P.Id==RetainedPhotoId;});};
 auto Ops=[&]{const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X,P.Z+1,0);};
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
  M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);
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
  if(!Check(F.Director->PlanPhotoControl(PhotoClaimId)&&F.HasFinding(TEXT("orb-control-plan.")+PhotoClaimId)&&F.Budget()==PhotoBudget,TEXT("developed original unlocks persisted S-03 reference plan without payment or manufactured control result")))return;
  if(!Check(!F.Director->PlanPhotoControl(PhotoClaimId)&&F.Director->RegisterPhotoClaim(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("orb plan is idempotent and documentation pays once only")))return;
  if(!Restore(TEXT("orb-paid-plan")))return;Go(9507,.5);return;
 }
 if(Stage==9507)
 {
  Ops();Go(9508,.3);return;
 }
 if(Stage==9508)
 {
  if(!Check(Original()&&Original()->IsDeveloped()&&Original()->Url==RetainedPhotoUrl&&Original()->Clock==S47J::Num(Snapshot,TEXT("photoClock"))&&F.Director->PhotoClaims().Num()==1&&F.Director->PhotoClaims()[0].bPaid&&F.HasFinding(TEXT("orb-control-plan.")+PhotoClaimId)&&!F.Director->PlanPhotoControl(PhotoClaimId)&&!F.Director->RegisterPhotoClaim(PhotoClaimId)&&F.Budget()==PhotoBudget+20,TEXT("disk Continue preserves real developed original, paid incident and unique uncompleted control plan")))return;
  Finish(true,TEXT("Forced native dusk orb: actual rendered exposure, physical development, dated reference plan, once-only credit and active/paid disk Continue. Placement/clock are explicit fixtures; no natural discovery or completed field comparison claimed."));
 }
}
