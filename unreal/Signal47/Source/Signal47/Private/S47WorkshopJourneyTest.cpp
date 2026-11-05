#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47FieldLogger.h"
#include "S47Workshop.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Startup.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

// One whole workshop journey: stock -> strip -> test -> refuse -> scrap -> build
// -> calibrate -> carry -> deploy -> retrieve -> take apart, with disk restores.
struct FS47WorkshopJourneyState { double Began=0;int32 Cash=0,Debt=0,Cells=0;bool bPanelShot=false; };

void FS47FreeformTest::StepWorkshopJourney(double Dt)
{
 if(!WorkshopJourney){WorkshopJourney=MakeShared<FS47WorkshopJourneyState>();WorkshopJourney->Began=FPlatformTime::Seconds();}
 auto& S=*WorkshopJourney;
 if(FPlatformTime::Seconds()-S.Began>300){Finish(false,TEXT("workshop journey exceeded its five-minute wall bound"));return;}
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.Field)return;auto& W=*F.Field;const auto& L=W.FieldLogger();const auto& C=W.Workshop();
 auto Approach=[&](const FString& Id)
 {
  M->UI->Close(true);const auto P=W.StandPoint(Id);const auto T=W.InteractionPoint(Id);M->Player->Place(P.X,P.Z,0);
  const FVector D=T-FVector(P.X,M->Player->FloorY+M->Player->Eye,P.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 };
 auto Use=[&](const FString& Id)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==Id&&bool(M->Player->OnUse),TEXT("actual physical eye-ray selects ")+Id+TEXT("; got ")+Pick))return false;
  M->Player->OnUse(Id);return true;
 };
 auto Click=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;
  TFunction<void(TSharedRef<SWidget>,bool)> Visit=[&](TSharedRef<SWidget> W2,bool)
  {
   if(W2->GetTypeAsString()==TEXT("SButton"))
   {
    bool Match=false;TFunction<void(TSharedRef<SWidget>)> Text=[&](TSharedRef<SWidget> T){if(T->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(T)->GetText().ToString().Contains(Label))Match=true;FChildren* Ch=T->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Text(Ch->GetChildAt(I));};
    Text(W2);if(Match)Found.Add(StaticCastSharedRef<SButton>(W2));
   }
   FChildren* Ch=W2->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Visit(Ch->GetChildAt(I),false);
  };
  if(auto Modal=M->Hud?M->Hud->ModalWidget():nullptr)Visit(Modal.ToSharedRef(),false);
  if(Found.Num()<1||!Found[0]->IsEnabled())return false;Found[0]->SimulateClick();return true;
 };
 auto Part=[&](const TCHAR* Kind,bool Faulty)->const FS47Workshop::FPart*{return C.Tray.FindByPredicate([&](const FS47Workshop::FPart& P){return P.Kind==Kind&&P.bTested&&P.bFaulty==Faulty;});};
 auto Untested=[&]()->const FS47Workshop::FPart*{return C.Tray.FindByPredicate([](const FS47Workshop::FPart& P){return !P.bTested;});};
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(!W.LoggerAccess()&&!L.bLoanIssued&&C.Tray.IsEmpty()&&C.Built.IsEmpty(),TEXT("fresh start: logger locked by case access, empty workshop")))return;
  S.Cash=F.Budget();S.Debt=F.Debt();W.AddItem(TEXT("recorder_donor"),2);W.AddItem(TEXT("cell"),3);W.AddItem(TEXT("foil"),1);
  Check(true,TEXT("DECLARED fixture: two recorders, three cells and one foil placed in stock; purchase and transport are covered by the existing parcel journey"));
  Approach(TEXT("ff:bench"));Go(10,.5);return;
 }
 if(Stage==10){if(!Use(TEXT("ff:bench")))return;Go(11,.4);return;}
 if(Stage==11)
 {
  if(!Check(Click(TEXT("Open workshop")),TEXT("actual workbench button opens the workshop")))return;Go(12,.4);return;
 }
 if(Stage==12)
 {
  if(!Check(C.Recipes().Num()==3&&!C.Known(*FS47Workshop::Recipe(TEXT("own_logger"))),TEXT("recipes are hidden before any part has been handled")))return;
  if(!Check(Click(TEXT("Strip a used recorder")),TEXT("actual strip button starts the bench job"))||!Check(W.BenchBusy()&&W.InventoryCount(TEXT("recorder_donor"))==1,TEXT("strip job holds the consumed recorder")))return;
  if(!Restore(TEXT("workshop-mid-strip")))return;Go(13,.4);return;
 }
 if(Stage==13)
 {
  if(W.BenchBusy())return;
  if(!Check(C.Tray.Num()==4&&C.Donors==1&&W.InventoryCount(TEXT("recorder_donor"))==1,TEXT("strip job survives disk restore and gives exactly four parts")))return;
  Approach(TEXT("ff:bench"));Go(14,.8);return;
 }
 if(Stage==14)
 {
  W.OpenWorkshop();Go(142,.4);return;
 }
 if(Stage==140){Shot(TEXT("workshop-tray-full.png"));Go(141,.4);return;}
 if(Stage==141){Approach(TEXT("ff:bench"));W.OpenWorkshop();Go(21,.5);return;}
 if(Stage==142)
 {
  Shot(TEXT("workshop-panel-untested.png"));
  if(!Check(Click(TEXT("Test all 4 untested"))&&W.BenchBusy(),TEXT("one button tests every untested part")))return;Go(15,.2);return;
 }
 if(Stage==15){if(W.BenchBusy())return;if(!Check(!Untested()&&M->UI->Modal(),TEXT("all parts tested in one job and the panel comes back by itself")))return;M->UI->Close(true);Go(16,.2);return;}
 if(Stage==16)
 {
  if(!Check(Part(TEXT("timer"),true)&&Part(TEXT("recorder"),false)&&Part(TEXT("housing"),false)&&Part(TEXT("lead"),false),TEXT("testing reveals this donor's dead timer and three good parts")))return;
  const FString Why=W.WorkshopBlock(TEXT("build"),TEXT("own_logger"));
  if(!Check(Why.Contains(TEXT("timer board")),TEXT("build is refused and names the exact missing part: ")+Why))return;
  if(!Check(!W.WorkshopAction(TEXT("build"),TEXT("own_logger"))&&!W.BenchBusy()&&C.Tray.Num()==4&&W.InventoryCount(TEXT("cell"))==3,TEXT("refused build consumes nothing")))return;
  if(!Check(W.WorkshopAction(TEXT("scrapdead"))&&C.Tray.Num()==3,TEXT("dead timer scrapped")))return;
  if(!Check(W.WorkshopAction(TEXT("unpack")),TEXT("second recorder stripped for a working timer")))return;Go(17,.2);return;
 }
 if(Stage==17)
 {
  if(W.BenchBusy())return;
  if(!Check(C.Tray.Num()==7,TEXT("second donor adds four parts")))return;Go(18,.1);return;
 }
 if(Stage==18){if(!Check(W.WorkshopAction(TEXT("testall")),TEXT("second batch goes to the test set")))return;Go(19,.2);return;}
 if(Stage==19){if(W.BenchBusy())return;Go(20,.1);return;}
 if(Stage==20)
 {
  if(!Check(Part(TEXT("recorder"),true)&&Part(TEXT("timer"),false),TEXT("second donor has its own fault: dead recorder, good timer")))return;
  {const auto P=W.StandPoint(TEXT("ff:bench"));const FVector T=W.InteractionPoint(TEXT("ff:bench"))+FVector(1.1,0,0);M->UI->Close(true);M->Player->Place(P.X+1.1,P.Z,0);const FVector D=T-FVector(P.X+1.1,M->Player->FloorY+M->Player->Eye,P.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());}
  Go(140,1.0);return;
 }
 if(Stage==21){Shot(TEXT("workshop-panel-parts.png"));Go(22,.5);return;}
 if(Stage==22)
 {
  if(!Check(Click(TEXT("Field logger")),TEXT("recipe list selects the logger")))return;Go(220,.3);return;
 }
 if(Stage==220)
 {
  if(!Check(Click(TEXT("Build")),TEXT("actual build button starts the selected logger")))return;
  if(!Check(!W.LoggerAccess(),TEXT("logger is not usable until the build has finished")))return;
  if(!Restore(TEXT("workshop-mid-build")))return;Go(23,.4);return;
 }
 if(Stage==23)
 {
  if(W.BenchBusy())return;
  if(!Check(C.IsBuilt(TEXT("own_logger"))&&L.bOwned&&W.LoggerAccess()&&L.Assembly==3&&!L.bCalibrated,TEXT("build survives restore; own logger opens field work without case access and still needs calibration")))return;
  if(!Check(W.InventoryCount(TEXT("cell"))==2&&C.Tray.Num()==4,TEXT("build spends exactly one cell and three parts")))return;
  Approach(TEXT("ff:bench"));Go(24,.4);return;
 }
 if(Stage==24){if(!Check(W.WorkshopAction(TEXT("build"),TEXT("shielded_lead")),TEXT("shielded lead build starts")))return;Go(25,.2);return;}
 if(Stage==25)
 {
  if(W.BenchBusy())return;
  if(!Check(L.bShieldedLead&&W.InventoryCount(TEXT("foil"))==0,TEXT("shielded lead fitted and foil spent")))return;
  // The second donor's housing is the only good one left; the first went into the logger.
  if(!Check(W.WorkshopAction(TEXT("build"),TEXT("battery_pack")),TEXT("battery pack build starts")))return;Go(26,.2);return;
 }
 if(Stage==26)
 {
  if(W.BenchBusy())return;
  if(!Check(L.bBatteryPack&&W.InventoryCount(TEXT("cell"))==0&&L.MaxBattery()==2*FS47FieldLogger::Capacity,TEXT("battery pack doubles capacity and spends two cells")))return;
  if(!Check(W.LoggerAction(TEXT("calibrate")),TEXT("existing bench calibration accepts the built logger")))return;Go(27,.2);return;
 }
 if(Stage==27)
 {
  if(!L.bCalibrated)return;
  if(!Check(L.Battery==L.MaxBattery(),TEXT("calibration charges to the extended capacity")))return;
  Approach(TEXT("ff:bench"));Go(28,.4);return;
 }
 if(Stage==28)
 {
  if(!Check(W.LoggerAction(TEXT("take"))&&W.ParcelStage()==2,TEXT("built logger is carried by hand from the bench")))return;
  const FString Why=W.WorkshopBlock(TEXT("strip"),TEXT("own_logger"));
  if(!Check(!Why.IsEmpty(),TEXT("a logger in your hands cannot be taken apart")))return;
  Approach(TEXT("ff:logger-a"));Go(29,.4);return;
 }
 if(Stage==29)
 {
  if(!Check(W.LoggerAction(TEXT("site0"))&&L.bRecording&&L.Tapes.Num()==1&&L.Tapes[0].bOwned&&L.Tapes[0].bShielded,TEXT("placed at site A; the tape records its operator-built, shielded provenance")))return;
  Go(30,1.0);return;
 }
 if(Stage==30)
 {
  if(!Check(L.Tapes[0].Seconds>0&&L.Battery<L.MaxBattery(),TEXT("real field sampling draws on the extended battery")))return;
  if(!Check(W.LoggerAction(TEXT("site0"))&&L.Site<0&&W.ParcelStage()==2&&L.Report(L.Tapes[0]).Contains(TEXT("OPERATOR-BUILT"))&&L.Report(L.Tapes[0]).Contains(TEXT("foil-shielded")),TEXT("retrieved original reports who built it and how it was shielded")))return;
  Approach(TEXT("ff:bench"));Go(31,.4);return;
 }
 if(Stage==31)
 {
  if(!Use(TEXT("ff:bench")))return; // returns the carried logger through the existing deposit
  Go(32,.4);return;
 }
 if(Stage==32)
 {
  M->UI->Close(true);
  if(!Check(!L.bCheckedOut&&W.ParcelStage()==0,TEXT("logger returned to the bench")))return;
  if(!Restore(TEXT("workshop-returned")))return;Go(33,.4);return;
 }
 if(Stage==33)
 {
  if(!Check(L.bOwned&&L.bShieldedLead&&L.bBatteryPack&&C.IsBuilt(TEXT("battery_pack"))&&L.Tapes.Num()==1&&L.Tapes[0].bRetrieved&&L.Tapes[0].bShielded&&W.LoggerAccess(),TEXT("instrument, fittings and original survive a third disk restore")))return;
  Approach(TEXT("ff:bench"));Go(34,.8);return;
 }
 if(Stage==34)
 {
  Shot(TEXT("workshop-tray-after.png"));Go(340,.4);return;
 }
 if(Stage==340)
 {
  Approach(TEXT("ff:bench"));
  const int32 Before=C.Tray.Num();S.Cells=Before;
  if(!Check(W.WorkshopBlock(TEXT("strip"),TEXT("battery_pack")).IsEmpty()&&W.WorkshopAction(TEXT("strip"),TEXT("battery_pack")),TEXT("battery pack strip starts with the logger home")))return;Go(35,.2);return;
 }
 if(Stage==35)
 {
  if(W.BenchBusy())return;
  if(!Check(!L.bBatteryPack&&L.Battery<=FS47FieldLogger::Capacity&&C.Tray.Num()==S.Cells+1&&C.Tray.Last().Kind==TEXT("housing")&&C.Tray.Last().Usable()&&W.InventoryCount(TEXT("cell"))==0,TEXT("taking the pack apart returns its housing once, not its cells, and clamps the charge")))return;
  if(!Check(F.Budget()==S.Cash&&F.Debt()==S.Debt,TEXT("the workshop itself moved no money")))return;
  Approach(TEXT("ff:bench"));W.OpenWorkshop();Go(36,.5);return;
 }
 if(Stage==36){Shot(TEXT("workshop-panel-built.png"));M->UI->Close(true);F.OpenArchive();Go(37,.6);return;}
 if(Stage==37)
 {
  Shot(TEXT("archive-records.png"));Go(38,.6);return;
 }
 if(Stage==38)
 {
  Finish(true,TEXT("Whole workshop journey: strip, test, refusal, scrap, three builds, calibration, carry, field placement, retrieval, return and take-apart with three disk restores. Declared stock and placement fixtures; no shop trip, full tape or human acceptance."));return;
 }
}
