#include "S47CaseFieldTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47FreeformTest.h"
#include "S47CaseFieldTests.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformWorld.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Three.h"
#include "Components/PrimitiveComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 TArray<TSharedRef<SWidget>> Roots(US47Main* M)
 {
  TArray<TSharedRef<SWidget>> Out;TSet<const SWidget*> Seen;
  if(FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& U)
  {auto W=U.GetFocusedWidget();if(!W)return;while(auto Parent=W->GetParentWidget())W=Parent;if(!Seen.Contains(W.Get())){Seen.Add(W.Get());Out.Add(W.ToSharedRef());}},true);
  if(Out.IsEmpty()&&M&&M->Hud)if(auto W=M->Hud->ModalWidget())Out.Add(W.ToSharedRef());return Out;
 }
 bool TextIn(const TSharedRef<SWidget>& W,const FString& Text,bool Exact)
 {
  if(!W->GetVisibility().IsVisible())return false;
  if(W->GetTypeAsString()==TEXT("STextBlock")){const FString S=StaticCastSharedRef<STextBlock>(W)->GetText().ToString();if(Exact?S==Text:S.Contains(Text))return true;}
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)if(TextIn(C->GetChildAt(I),Text,Exact))return true;return false;
 }
 void ButtonsIn(const TSharedRef<SWidget>& W,const FString& Text,TArray<TSharedRef<SButton>>& Out)
 {
  if(!W->GetVisibility().IsVisible())return;
  if(W->GetTypeAsString()==TEXT("SButton")&&TextIn(W,Text,true))Out.AddUnique(StaticCastSharedRef<SButton>(W));
  auto* C=W->GetChildren();for(int32 I=0;C&&I<C->Num();++I)ButtonsIn(C->GetChildAt(I),Text,Out);
 }
}
bool FS47CaseFieldTest::Click(const FString& Label)
{
 TArray<TSharedRef<SButton>> Buttons;for(const auto& W:Roots(M))ButtonsIn(W,Label,Buttons);
 if(!O.Check(Buttons.Num()==1&&Buttons[0]->IsEnabled(),TEXT("unique enabled case-control native button: ")+Label))return false;
#if !UE_BUILD_SHIPPING
 Buttons[0]->SimulateClick();return true;
#else
 O.Finish(false,TEXT("native case controls require a development build"));return false;
#endif
}
bool FS47CaseFieldTest::HasText(const FString& Text)const
{for(const auto& W:Roots(M))if(TextIn(W,Text,false))return true;return false;}
void FS47CaseFieldTest::Pose(const FVector& Stand,const FVector& Target)
{
 M->UI->Close(true);M->Player->Place(Stand.X,Stand.Z,0);
 const FVector D=Target-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}
void FS47CaseFieldTest::ApproachControl(int32 Test)
{
 const auto* I=M->Inter->Get(Test==0?TEXT("case:roof-plate"):TEXT("case:gate-seal"));
 if(!I||!I->Object){O.Check(false,TEXT("physical control object exists before approach"));return;}
 const FVector P=I->Object.WorldPosition();Pose(P+FVector(0,0,Test==0?-1.7:1.05),P);
}
void FS47CaseFieldTest::Board()
{
 M->UI->Close(true);const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));
 M->Player->Place(P.X,P.Z+1.1,0);M->Freeform->Use(TEXT("opsTerminal"));M->Freeform->OpenCaseBoard(2);
}
bool FS47CaseFieldTest::UseRay(const FString& Id)
{
 const FString Picked=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
 UE_LOG(LogTemp,Display,TEXT("S47 CaseField ray picked=%s expected=%s eye=%s"),*Picked,*Id,*S47Three::ToThree(M->Player->EyeUE()).ToString());
 if(!O.Check(Picked==Id&&!M->Inter->LabelOf(Id).IsEmpty()&&bool(M->Player->OnUse),TEXT("actual player ray and usable label reach ")+Id))return false;
 M->Player->OnUse(Id);return true;
}
void FS47CaseFieldTest::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Freeform->CaseField||!M->Player||!M->World||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;auto& C=*F.CaseField;
 const bool Preview=FParse::Param(FCommandLine::Get(),TEXT("S47CaseFieldPreview"));
 if(Preview&&O.Age>45){O.Finish(false,TEXT("bounded roof preparation preview timeout"));return;}
 auto Check=[&](bool Good,const TCHAR* Why){return O.Check(Good,Why);};
 if(O.Stage==0)
 {
  if(O.Age<3)return;
  if(!Check(M->Flow->bDevRun&&M->World->Area==TEXT("saro"),TEXT("isolated native SARO field-control fixture")))return;
  M->UI->Close(true);F.Director->Stop();
  int32 Seed=1;FS47FreeformCases Cases;for(;Seed<1000;++Seed){Cases.Reset(Seed);if(Cases.Get(2)->Truth==1)break;}
  if(!Check(Seed<1000&&Cases.Open(2,1,20*3600.),TEXT("explicit seeded people-cause fixture opens Visitors without injecting test results")))return;
  auto State=S47J::Read(S47J::Write(F.Save()));S47J::Set(State,TEXT("jobSeed"),Seed);S47J::Set(State,TEXT("tutorial"),0);S47J::Set(State,TEXT("clock"),20*3600.);S47J::Set(State,TEXT("cases"),Cases.Save());
  F.Begin(State,false);F.Director->Stop();Board();O.Go(Preview?9680:9600,.5);return;
 }
 if(O.Stage==9680)
 {
  if(!Click(TEXT("Plan this test: Secure the roof fitting")))return;
  ApproachControl(0);M->UI->ClearToasts();O.Go(9681,2);return;
 }
 if(O.Stage==9681)
 {
  if(!Check(F.PlannedFieldTest(2)==0&&!C.Prepared(),TEXT("preview shows the physical roof control before preparation")))return;
  O.Shot(TEXT("casefield-preview-roof-before"));O.Go(9682,.6);return;
 }
 if(O.Stage==9682)
 {
  if(!UseRay(TEXT("case:roof-plate")))return;
  if(!Check(C.Prepared()&&C.Window(Start,End)&&S47J::Num(C.Save(),TEXT("wind"))>0,TEXT("preview uses actual roof preparation and retained wind reference")))return;
  M->UI->ClearToasts();O.Go(9683,2);return;
 }
 if(O.Stage==9683){O.Shot(TEXT("casefield-preview-roof-after"));O.Go(9684,.6);return;}
 if(O.Stage==9684){Board();O.Go(96841,.4);return;}
 if(O.Stage==96841){if(!Click(TEXT("Cancel this field plan")))return;O.Go(9685,.4);return;}
 if(O.Stage==9685)
 {
  if(!Click(TEXT("Plan this test: Controlled access")))return;
  ApproachControl(1);M->UI->ClearToasts();O.Go(9686,.5);return;
 }
 if(O.Stage==9686)
 {
  if(!UseRay(TEXT("case:gate-seal")))return;
  if(!Check(C.Prepared(),TEXT("geometry preview prepares real gate before explicit disturbance fixture")))return;
  auto Fixture=C.Save();S47J::Set(Fixture,TEXT("gateCrossed"),true);C.Load(Fixture);
  UE_LOG(LogTemp,Display,TEXT("S47 PREVIEW FIXTURE: gateCrossed injected for geometry only; no active window or result claim"));
  Pose(FVector(-1,0,-17.7),FVector(-1,-.3,-20));M->UI->ClearToasts();O.Go(9687,2);return;
 }
 if(O.Stage==9687){O.Shot(TEXT("casefield-preview-gate-disturbance-fixture"));O.Go(9688,.6);return;}
 if(O.Stage==9688){O.Finish(true,TEXT("Roof preparation and explicit gate disturbance geometry rendered; visual review required, no window completion claimed."));return;}
 if(O.Stage==9600)
 {
  if(!Check(F.TerminalOnGlass()&&HasText(TEXT("Visitors around the base"))&&HasText(TEXT("OPTIONAL WORKING HUNCH")),TEXT("native case board shows compact controls and optional hunch separately")))return;
  O.Shot(TEXT("casefield-board-before"));if(!Click(TEXT("Plan this test: Controlled access")))return;O.Go(9601,.4);return;
 }
 if(O.Stage==9601)
 {
  double Target=0;
  if(!Check(F.PlannedFieldTest(2)==1&&!C.Prepared()&&!F.CaseWaitTarget(2,Target)&&F.NextAction().Contains(TEXT("GATE")),TEXT("unprepared gate plan is focused with physical instructions and no premature wait shortcut")))return;
  ApproachControl(1);O.Go(9602,.4);return;
 }
 if(O.Stage==9602)
 {
  O.Shot(TEXT("casefield-gate-before"));O.Go(96021,.3);return;
 }
 if(O.Stage==96021)
 {
  if(!UseRay(TEXT("case:gate-seal")))return;
  if(!Check(C.Prepared()&&C.Window(Start,End)&&End-Start==10800&&!M->Web->IsDoorOpen(TEXT("fieldGate"))&&!C.Prepare(1),TEXT("physical strip action closes and seals real gate once with a three-hour window")))return;
  // Real door-controller action, explicit approach fixture; no coverage is injected.
  if(!Check(M->Web->ToggleDoor(TEXT("fieldGate")),TEXT("pre-window disturbance opens the real field gate")))return;
  C.Step(.01);M->Web->SetDoorOpen(TEXT("gate"),false,true);
  if(!Check(S47J::Bool(C.Save(),TEXT("gateTampered"))&&C.Objective().Contains(TEXT("DISTURBED"))&&!C.Prepare(1),TEXT("pre-window opening permanently breaks preparation; closing never auto-reseals")))return;
  if(!O.Restore(TEXT("casefield-prewindow-disturbed")))return;O.Go(96022,.4);return;
 }
 if(O.Stage==96022)
 {
  F.Director->Stop();
  if(!Check(C.Prepared()&&S47J::Bool(C.Save(),TEXT("gateTampered"))&&!F.CaseFile().Get(2)->Results[1].bDone,TEXT("pre-window disturbed seal survives disk Continue without a result")))return;
  if(!Check(F.CancelFieldTest(2)&&F.PlanCaseTest(2,1),TEXT("player can explicitly cancel and plan a new control after disturbance")))return;
  ApproachControl(1);O.Go(96023,.4);return;
 }
 if(O.Stage==96023)
 {
  if(!UseRay(TEXT("case:gate-seal")))return;
  if(!Check(C.Prepared()&&C.Window(Start,End)&&!S47J::Bool(C.Save(),TEXT("gateTampered"))&&S47J::Num(C.Save(),TEXT("coverage"))==0,TEXT("new physical preparation restores an intact seal without borrowed coverage")))return;
  PreparedAt=S47J::Num(C.Save(),TEXT("prepared"));Board();O.Go(9603,.4);return;
 }
 if(O.Stage==9603)
 {
  if(!Check(HasText(TEXT("SHIFT DAY"))&&HasText(TEXT("01:30"))&&HasText(TEXT("04:30")),TEXT("prepared case board names concrete shift day, start and end")))return;
  if(!Click(TEXT("Wait until five minutes before this window / review consequences")))return;O.Go(9604,.4);return;
 }
 if(O.Stage==9604)
 {
  if(!Check(HasText(TEXT("GENERATOR"))&&HasText(TEXT("FIELD LOGGER"))&&HasText(TEXT("No observing")),TEXT("native wait confirmation explains power, battery and missing observation consequences")))return;
  O.Shot(TEXT("casefield-wait-consequences"));if(!Click(TEXT("Wait until the stated time")))return;O.Go(9605,.3);return;
 }
 if(O.Stage==9605)
 {
  if(!Check(F.Clock()>=Start-300&&F.Clock()<Start-280&&S47J::Num(C.Save(),TEXT("gaps"))==0&&!F.CaseFile().Get(2)->Results[1].bDone,TEXT("safe wait stops before the prepared window without inventing gate coverage or result")))return;
  if(!O.Restore(TEXT("casefield-prepared-gate")))return;O.Go(9606,.4);return;
 }
 if(O.Stage==9606)
 {
  F.Director->Stop();double A=0,B=0;
  if(!Check(C.Prepared()&&C.Window(A,B)&&A==Start&&B==End&&S47J::Num(C.Save(),TEXT("prepared"))==PreparedAt,TEXT("disk Continue preserves the exact prepared gate and its dated window")))return;
  M->UI->Close(true);if(!Check(F.Rest()&&F.Clock()>End&&S47J::Num(C.Save(),TEXT("gaps"))==10800&&S47J::Num(C.Save(),TEXT("coverage"))==0,TEXT("production morning rest records the full missed gate window before saving")))return;
  if(!O.Restore(TEXT("casefield-skipped-window")))return;O.Go(9607,.4);return;
 }
 if(O.Stage==9607)
 {
  F.Director->Stop();
  if(!Check(S47J::Num(C.Save(),TEXT("gaps"))==10800&&!F.CaseFile().Get(2)->Results[1].bDone&&C.Objective().Contains(TEXT("incomplete")),TEXT("Continue retains incomplete control, not a fabricated negative or reward")))return;
  ApproachControl(1);O.Go(9608,.4);return;
 }
 if(O.Stage==9608)
 {
  O.Shot(TEXT("casefield-gate-incomplete"));if(!UseRay(TEXT("case:gate-seal")))return;
  // CASETEST-20261109: an invalid control stays planned to take again, fresh (no coverage borrowed); this run then plans the roof instead
  if(!Check(!F.CaseFile().Get(2)->Results[1].bDone&&F.CaseFile().Get(2)->ValidTests==0&&F.PlannedFieldTest(2)==1&&!C.Prepared()&&S47J::Num(C.Save(),TEXT("gaps"))==0&&F.FeedbackText().Contains(TEXT("stays planned")),TEXT("physical inspection of slept-through control records no conclusion and keeps the test planned, fresh, to take again")))return;
  if(!Check(F.CancelFieldTest(2)&&F.PlannedFieldTest(2)==-1,TEXT("the kept plan can still be cleared for another control")))return;
  Board();O.Go(9610,.4);return;
 }
 if(O.Stage==9610)
 {
  if(!Click(TEXT("Plan this test: Secure the roof fitting")))return;O.Go(9611,.3);return;
 }
 if(O.Stage==9611){ApproachControl(0);O.Go(9612,.4);return;}
 if(O.Stage==9612)
 {
  O.Shot(TEXT("casefield-roof-before"));O.Go(96121,.3);return;
 }
 if(O.Stage==96121)
 {
  if(!UseRay(TEXT("case:roof-plate")))return;
  if(!Check(C.Prepared()&&C.Window(Start,End)&&S47J::Num(C.Save(),TEXT("wind"))>0&&C.Objective().Contains(TEXT("CONTROL ROOM")),TEXT("physical roof fastening retains actual wind reference and points to the control room")))return;
  bRoofPresent=F.FieldTestExpectsPresent(2,0);if(!Check(bRoofPresent,TEXT("chosen seed requires physically audible impacts after roof fastening")))return;
  M->UI->Close(true);if(!Check(F.WaitUntil(Start)&&S47J::Num(C.Save(),TEXT("listened"))==0&&S47J::Num(C.Save(),TEXT("gaps"))==0,TEXT("explicit pre-window wait supplies no listening and does not skip the window")))return;
  Pose(FVector(0,0,0),FVector(0,2.5,-2));O.Go(9613);return;
 }
 if(O.Stage==9613)
 {
  if(S47J::Num(C.Save(),TEXT("listened"))<15){if(O.Age-O.At>120)Check(false,TEXT("comparable wind permits initial active listening within two minutes"));return;}
  Listened=S47J::Num(C.Save(),TEXT("listened"));PauseClock=F.Clock();M->bPausedByMenu=true;O.Go(9614,2);return;
 }
 if(O.Stage==9614)
 {
  const bool Held=F.Clock()==PauseClock&&S47J::Num(C.Save(),TEXT("listened"))==Listened;M->bPausedByMenu=false;
  if(!Check(Held,TEXT("menu pause freezes both observation clock and roof listening")))return;
  Pose(FVector(10,0,-7),FVector(5,2,-5));Listened=S47J::Num(C.Save(),TEXT("listened"));O.Go(9615,2);return;
 }
 if(O.Stage==9615)
 {
  if(!Check(S47J::Num(C.Save(),TEXT("listened"))==Listened&&!F.CaseFile().Get(2)->Results[0].bDone,TEXT("leaving the control room cannot add listening or finish audible roof evidence")))return;
  Pose(FVector(0,0,0),FVector(0,2.5,-2));Sample=S47J::Read(S47J::Write(C.Save()));
  if(!O.Restore(TEXT("casefield-roof-listening")))return;O.Go(9616,.4);return;
 }
 if(O.Stage==9616)
 {
  F.Director->Stop();const auto Saved=C.Save();
  if(!Check(S47J::Num(Saved,TEXT("listened"))>=S47J::Num(Sample,TEXT("listened"))&&S47J::Num(Saved,TEXT("listened"))<S47J::Num(Sample,TEXT("listened"))+2&&S47J::Num(Saved,TEXT("wind"))==S47J::Num(Sample,TEXT("wind"))&&S47J::Int(Saved,TEXT("impacts"))>=S47J::Int(Sample,TEXT("impacts")),TEXT("disk Continue preserves partial listening, emitted impacts and the original wind reference")))return;
  Pose(FVector(0,0,0),FVector(0,2.5,-2));O.Shot(TEXT("casefield-roof-listening"));O.Go(9617);return;
 }
 if(O.Stage==9617)
 {
  const auto* Case=F.CaseFile().Get(2);if(!Case->Results[0].bDone){if(O.Age-O.At>210)Check(false,TEXT("timed wind-qualified roof control finishes within the real window"));return;}
  if(!Check(Case->Results[0].bPresent==bRoofPresent&&Case->ValidTests==1&&F.HasFinding(TEXT("case3-test1")),TEXT("real active roof listening records the seeded audible result exactly once")))return;
  Board();O.Go(9620,.4);return;
 }
 if(O.Stage==9620)
 {
  if(!Click(TEXT("Plan this test: Controlled access")))return;O.Go(9621,.4);return;
 }
 if(O.Stage==9621){ApproachControl(1);O.Go(9622,.4);return;}
 if(O.Stage==9622)
 {
  if(!UseRay(TEXT("case:gate-seal")))return;
  if(!Check(C.Prepared()&&C.Window(Start,End)&&S47J::Num(C.Save(),TEXT("coverage"))==0&&S47J::Num(C.Save(),TEXT("gaps"))==0,TEXT("replanned gate control starts fresh instead of borrowing the slept-through preparation")))return;
  bGatePresent=F.FieldTestExpectsPresent(2,1);if(!Check(bGatePresent,TEXT("chosen seed requires a staged gate disturbance during the active window")))return;
  M->UI->Close(true);if(!Check(F.WaitUntil(Start),TEXT("production wait reaches start without skipping any positive gate-control time")))return;
  Pose(FVector(0,0,0),FVector(0,2,-3));bGateRestored=false;O.Go(9623);return;
 }
 if(O.Stage==9623)
 {
  const auto Saved=C.Save();Coverage=S47J::Num(Saved,TEXT("coverage"));
  if(!bGateRestored&&Coverage>=3600)
  {
   Sample=S47J::Read(S47J::Write(Saved));if(!O.Restore(TEXT("casefield-gate-active-hour")))return;bGateRestored=true;O.Go(9624,.4);return;
  }
  if(F.Clock()<End+.1){if(O.Age-O.At>600)Check(false,TEXT("three-hour gate window advances through bounded actual game ticks"));return;}
  if(!Check(Coverage>=10800-.01&&S47J::Num(Saved,TEXT("gaps"))==0&&S47J::Bool(Saved,TEXT("gateCrossed"))==bGatePresent&&!S47J::Bool(Saved,TEXT("gateTampered")),TEXT("entire gate window is actually sampled; event is staged without sleep or operator disturbance")))return;
  if(!Check(!F.CaseFile().Get(2)->Results[1].bDone,TEXT("completed passive gate control still requires physical inspection")))return;
  // The control is already complete. Daylight improves the result picture without adding coverage.
  const double Morning=(FMath::FloorToDouble(F.Clock()/86400.)*86400.)+8*3600.;
  if(Morning>F.Clock())if(!Check(F.WaitUntil(Morning),TEXT("post-window daylight wait leaves completed control intact")))return;
  ApproachControl(1);O.Go(9625,.5);return;
 }
 if(O.Stage==9624)
 {
  F.Director->Stop();const auto Saved=C.Save();
  if(!Check(C.Prepared()&&S47J::Num(Saved,TEXT("coverage"))>=S47J::Num(Sample,TEXT("coverage"))&&S47J::Num(Saved,TEXT("coverage"))<S47J::Num(Sample,TEXT("coverage"))+30&&S47J::Num(Saved,TEXT("gaps"))==0,TEXT("disk Continue keeps a partly observed gate window without losing coverage or inventing a gap")))return;
  Pose(FVector(0,0,0),FVector(0,2,-3));O.Go(9623);return;
 }
 if(O.Stage==9625)
 {
  TArray<UPrimitiveComponent*> Components;M->Web->GetComponents<UPrimitiveComponent>(Components);bool Seal=false,Track=false;
  for(auto* P:Components)if(P&&P->IsVisible()){Seal|=P->GetName().StartsWith(TEXT("CaseGateSealBroken"));Track|=P->GetName().StartsWith(TEXT("CaseGateTrack0"));}
  if(!Check(Seal&&Track,TEXT("native broken seal and physical track geometry are visible before inspection")))return;
  O.Shot(TEXT("casefield-gate-after-active-window"));O.Go(96251,.3);return;
 }
 if(O.Stage==96251)
 {
  // Fork the actually completed control for a destructive disturbance regression.
  // Restoring this captured branch below is explicit harness rollback, not new coverage.
  GateBranch=S47J::Read(S47J::Write(F.Save()));
  if(!Check(M->Web->ToggleDoor(TEXT("fieldGate")),TEXT("post-window disturbance opens the real gate before inspection")))return;
  C.Step(.01);M->Web->SetDoorOpen(TEXT("gate"),false,true);
  if(!Check(S47J::Bool(C.Save(),TEXT("gateTampered"))&&S47J::Num(C.Save(),TEXT("coverage"))>=10800-.01,TEXT("opening after full coverage invalidates seal while retaining actual coverage")))return;
  if(!O.Restore(TEXT("casefield-postwindow-disturbed")))return;O.Go(96252,.4);return;
 }
 if(O.Stage==96252)
 {
  F.Director->Stop();ApproachControl(1);
  if(!Check(S47J::Bool(C.Save(),TEXT("gateTampered"))&&!F.CaseFile().Get(2)->Results[1].bDone,TEXT("post-window disturbance survives disk Continue before inspection")))return;
  O.Go(96253,.3);return;
 }
 if(O.Stage==96253)
 {
  const int32 Relays=F.InventoryCount(TEXT("relay"));
  if(!UseRay(TEXT("case:gate-seal")))return;
  if(!Check(!F.CaseFile().Get(2)->Results[1].bDone&&F.CaseFile().Get(2)->ValidTests==1&&F.PlannedFieldTest(2)==1&&!C.Prepared()&&F.InventoryCount(TEXT("relay"))==Relays,TEXT("disturbed post-window inspection yields no valid result or reward and keeps the test planned to take again (CASETEST-20261109)")))return;
  F.Begin(GateBranch,false);F.Director->Stop();ApproachControl(1);O.Go(96254,.3);return;
 }
 if(O.Stage==96254)
 {
  if(!Check(C.Prepared()&&!S47J::Bool(C.Save(),TEXT("gateTampered"))&&S47J::Num(C.Save(),TEXT("coverage"))>=10800-.01,TEXT("explicit harness rollback restores separately captured undisturbed completed branch")))return;
  if(!UseRay(TEXT("case:gate-seal")))return;
  const auto* Case=F.CaseFile().Get(2);
  if(!Check(Case->Results[1].bDone&&Case->Results[1].bPresent==bGatePresent&&Case->ValidTests==2&&F.HasFinding(TEXT("case3-test2")),TEXT("native physical inspection retains the actual staged gate result and second valid control")))return;
  const int32 Relays=F.InventoryCount(TEXT("relay"));
  if(!Check(!C.InspectGate()&&F.CaseFile().Get(2)->ValidTests==2&&F.InventoryCount(TEXT("relay"))==Relays,TEXT("repeated inspection cannot record or reward the same gate control again")))return;
  if(!O.Restore(TEXT("casefield-completed-controls")))return;O.Go(9626,.4);return;
 }
 if(O.Stage==9626)
 {
  F.Director->Stop();const auto* Case=F.CaseFile().Get(2);
  if(!Check(Case->Results[0].bDone&&Case->Results[0].bPresent==bRoofPresent&&Case->Results[1].bDone&&Case->Results[1].bPresent==bGatePresent&&Case->ValidTests==2&&!C.Prepared(),TEXT("final Continue retains both observed controls without an active duplicate plan")))return;
  Board();O.Go(9627,.4);return;
 }
 if(O.Stage==9627)
 {
  if(!Check(HasText(TEXT("RESULT DAY"))&&HasText(TEXT("Register supported conclusion:")),TEXT("native case board presents retained results and only the now-supported conclusion")))return;
  O.Shot(TEXT("casefield-final-caseboard"));O.Finish(true,TEXT("Physical field controls, actual roof listening/full gate window, pre- and post-window seal disturbance, explicit saved-branch rollback, native planning/waiting, invalid sleep and disk Continue passed."));return;
 }
}
