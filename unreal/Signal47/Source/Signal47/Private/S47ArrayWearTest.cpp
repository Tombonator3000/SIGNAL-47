#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47ArrayWear.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"

// A new Freeform game on the worn station, restored branch by branch to a whole array.
void FS47FreeformTest::StepArrayWear(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>280){Finish(false,TEXT("array wear journey exceeded its time bound"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.Field)return;auto& W=*F.Field;const auto& A=W.ArrayWear();
 auto Dish=[&](const TCHAR* Id){for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==Id)return I;return -1;};
 auto Approach=[&](const TCHAR* Id)
 {
  M->UI->Close(true);const FVector T=W.InteractionPoint(FString(TEXT("field:"))+Id);M->Player->Place(T.X+1.6,T.Z+.8,0);
  const FVector D=T-FVector(T.X+1.6,M->Player->FloorY+M->Player->Eye,T.Z+.8);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 };
 auto Use=[&](const TCHAR* Id)
 {
  const FString Want=FString(TEXT("field:"))+Id;const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==Want&&bool(M->Player->OnUse),TEXT("actual physical eye-ray selects ")+Want+TEXT("; got ")+Pick))return false;
  M->Player->OnUse(Want);return true;
 };
 auto Click=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;
  TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> X)
  {
   if(X->GetTypeAsString()==TEXT("SButton"))
   {
    bool Match=false;TFunction<void(TSharedRef<SWidget>)> Text=[&](TSharedRef<SWidget> T){if(T->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(T)->GetText().ToString().Contains(Label))Match=true;FChildren* Ch=T->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Text(Ch->GetChildAt(I));};
    Text(X);if(Match)Found.AddUnique(StaticCastSharedRef<SButton>(X));
   }
   FChildren* Ch=X->GetChildren();for(int32 I=0;Ch&&I<Ch->Num();++I)Visit(Ch->GetChildAt(I));
  };
  if(auto Modal=M->Hud?M->Hud->ModalWidget():nullptr)Visit(Modal.ToSharedRef());
  // D45-20261110: a terminal page is drawn on the CRT's glass, in its own window
  if(F.TerminalOnGlass()&&FSlateApplication::IsInitialized())FSlateApplication::Get().ForEachUser([&](FSlateUser& User)
  {auto Widget=User.GetFocusedWidget();if(!Widget)return;while(auto Parent=Widget->GetParentWidget())Widget=Parent;Visit(Widget.ToSharedRef());},true);
  if(Found.Num()<1||!Found[0]->IsEnabled())return false;Found[0]->SimulateClick();return true;
 };
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 static const TCHAR* Junctions[]={TEXT("S-13"),TEXT("S-16"),TEXT("S-14"),TEXT("S-17")};
 const int32 B=Stage>=100?(Stage-100)/10:0;const int32 Sub=Stage>=100?(Stage-100)%10:0;
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(A.bActive&&F.AvailableDishCount()==12,TEXT("a new game starts on twelve of twenty-seven antennas")))return;
  if(!Check(FMath::IsNearlyEqual(F.ArrayMeasurementRate(),FS47ArrayWear::RateForCount(12))&&F.ArrayMeasurementRate()>.4&&F.ArrayMeasurementRate()<.5,TEXT("twelve antennas measure slowly but not unbearably")))return;
  if(!Check(!M->Web->DishRemoteAvailable(Dish(TEXT("S-27")))&&M->Web->DishRemoteAvailable(Dish(TEXT("S-03")))&&M->Web->DishEl(Dish(TEXT("S-27")))<15,TEXT("outer antennas are out of service and parked low; S-03 still works")))return;
  if(!Check(!W.TriggerServiceFault(false,TEXT("S-27")),TEXT("an antenna already out of service cannot take a second fault")))return;
  if(!Check(W.InventoryCount(TEXT("fuse"))==3&&W.BranchNeeds(*FS47ArrayWear::Branch(TEXT("west"))).IsEmpty(),TEXT("the first repair is possible from the station's own stock")))return;
  bool Pointed=F.Jobs().Num()>0;for(const auto& J:F.Jobs())Pointed&=!F.JobLocation(J).Contains(TEXT("ARRAY STATUS"));
  if(!Check(Pointed,TEXT("working on twelve antennas does not send job locations to ARRAY STATUS")))return;
  Approach(Junctions[0]);Go(5,.8);return;
 }
 if(Stage==5){Shot(TEXT("wear-parked-antenna.png"));Go(6,.5);return;}
 if(Stage==6){M->UI->Close(true);const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));M->Player->Place(P.X+1.,P.Z,PI/2);Go(7,.6);return;}
 if(Stage==7){if(M->Player->OnUse)M->Player->OnUse(TEXT("opsTerminal"));Go(70,2.2);return;}
 if(Stage==70){F.OpenServiceGuide();Go(71,1.0);return;}
 if(Stage==71){Shot(TEXT("wear-ops-table.png"));Go(72,.6);return;}
 // D45-20261110 (Tom: the player picks which branch is mended next): MAINTENANCE on the array PC, the station goal and the field lamps
 if(Stage==72){M->UI->Close(true);const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("crtLeft")));M->Player->Place(P.X,P.Z+1.1,0);F.Use(TEXT("crtLeft"));Go(73,1.2);return;}
 if(Stage==73){if(!Check(Click(TEXT("MAINTENANCE")),TEXT("the array PC has a MAINTENANCE tab")))return;Go(74,.6);return;}
 if(Stage==74)
 {
  if(!Check(A.NextBranch()==FS47ArrayWear::Branch(TEXT("west"))&&A.Ordered().Num()==4,TEXT("an untouched plan is the table order: the west branch is next")))return;
  if(!Check(Click(TEXT("FAR BRANCH")),TEXT("the far branch's row can be picked on MAINTENANCE")))return;Go(75,.4);return;
 }
 if(Stage==75)
 {
  if(!Check(Click(TEXT("MAKE THIS NEXT")),TEXT("MAKE THIS NEXT on the picked branch")))return;
  if(!Check(A.NextBranch()==FS47ArrayWear::Branch(TEXT("far"))&&F.StationGoalLine().Contains(TEXT("FAR BRANCH")),TEXT("the player's pick is the station goal")))return;
  const int32 Far=Dish(TEXT("S-17")),West=Dish(TEXT("S-13")),Whole=Dish(TEXT("S-03"));
  if(!Check(W.PilotState(Far)==3&&W.PilotState(West)==1&&W.PilotState(Whole)==0,FString::Printf(TEXT("cabinet pilots: the next one blinks red, other dead ones are red, a working one green (S-17 %d, S-13 %d, S-03 %d)"),W.PilotState(Far),W.PilotState(West),W.PilotState(Whole))))return;
  if(!Check(F.TerminalLines(0).ContainsByPredicate([](const FString& Line){return Line.StartsWith(TEXT("MAINTENANCE /"))&&Line.EndsWith(TEXT("NEXT S-17"));}),TEXT("the array PC's glass names the player's pick")))return;
  Go(76,.6);return;
 }
 if(Stage==76){Shot(TEXT("wear-maintenance.png"));if(!Restore(TEXT("wear-plan")))return;Go(77,.6);return;}
 if(Stage==77)
 {
  if(!Check(A.NextBranch()==FS47ArrayWear::Branch(TEXT("far"))&&A.Ordered().Num()==4&&A.Ordered()[1]==FS47ArrayWear::Branch(TEXT("west")),TEXT("the operator's order survives a disk restore")))return;
  Go(8,.2);return;
 }
 if(Stage==8){M->UI->Close(true);Go(100,.3);return;}
 if(Stage>=100&&B<4)
 {
  const FS47ArrayWear::FBranch& Br=FS47ArrayWear::Branches()[B];const int32 Before=27-A.DownCount();
  if(Sub==0)
  {
   if(B==1)Check(true,TEXT("DECLARED fixture: relays, splice and motor placed in stock from here; the shop and workshop routes are tested separately"));
   if(B>=1)for(const auto& N:Br.Needs)W.AddItem(N.Item,N.Count);
   Approach(Junctions[B]);Go(Stage+1,.5);return;
  }
  if(Sub==1){if(!Use(Junctions[B]))return;Go(Stage+1,.4);return;}
  if(Sub==2)
  {
   if(B==0)Shot(TEXT("wear-branch-panel.png"));Go(Stage+1,.4);return;
  }
  if(Sub==3)
  {
   if(!Check(Click(TEXT("Repair and test")),TEXT("actual repair button starts ")+FString(Br.Id)))return;
   if(!Check(!A.Job.IsEmpty()&&!A.IsRestored(Br.Id),TEXT("nothing is restored or spent until the test finishes")))return;
   if(B==0){if(!Restore(TEXT("wear-mid-repair")))return;Approach(Junctions[0]);}
   Go(Stage+1,.2);return;
  }
  if(Sub==4)
  {
   if(!A.Job.IsEmpty())return;
   static const int32 Expect[]={15,19,23,27};
   if(!Check(A.IsRestored(Br.Id)&&F.AvailableDishCount()==Expect[B],FString::Printf(TEXT("%s back in service: %d antennas"),Br.Id,Expect[B])))return;
   if(B==0&&!Check(W.InventoryCount(TEXT("fuse"))==1,TEXT("the repair spent exactly its two fuses, after surviving a disk restore")))return;
   if(!Check(M->Web->DishRemoteAvailable(Dish(Br.Dishes[0])),TEXT("repaired antennas answer remote control again")))return;
   Go(100+(B+1)*10,.3);if(B==3)Go(50,.3);return;
  }
 }
 if(Stage==50)
 {
  if(!Check(A.DownCount()==0&&!A.NextBranch()&&FMath::IsNearlyEqual(F.ArrayMeasurementRate(),1.)&&F.HasFinding(TEXT("service-branch-far")),TEXT("whole array measures at full rate and the repairs are on record")))return;
  if(!Restore(TEXT("wear-whole")))return;Go(51,.4);return;
 }
 if(Stage==51)
 {
  if(!Check(A.bActive&&A.DownCount()==0&&F.AvailableDishCount()==27,TEXT("restored array survives a second disk restore")))return;
  Go(52,.1);return;
 }
 if(Stage==52){Finish(true,TEXT("Worn start, four branch repairs through physical junctions and the actual button, two disk restores, whole array at full rate. Declared stock and placement; no drive to the antennas or human pacing."));return;}
}
