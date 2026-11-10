#include "S47Freeform.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47EastWing.h"

namespace
{
 bool AtDesk(US47Main* M,const TCHAR* Prop)
 {
  if(!M||!M->Web||!M->Player||M->Web->Area()!=TEXT("saro"))return false;
  const FVector U=M->Web->PropLocation(Prop);if(U.IsNearlyZero())return false;
  const FVector P=S47Three::ToThree(U);
  return FVector2D::Distance(FVector2D(P.X,P.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<3.5&&FMath::Abs(P.Y-M->Player->FloorY)<2.5;
 }
 bool AtOperations(US47Main* M){return AtDesk(M,TEXT("opsTerminal"));}
 // D45-20261110 (Tom: maintenance is done on the array PC): work orders are issued and signed at ARRAY STATUS. Signing at OPS still
 // works for this build, so an order taken at OPS in d45 can be finished where the player expects.
 bool AtServiceDesk(US47Main* M){return AtDesk(M,TEXT("crtLeft"));}
}
bool FS47FreeformSession::StartServiceGuide(bool bSupplyTrip)
{
 if(!bActive||ServiceGuide==1||!Field)return false;
 if(!AtServiceDesk(M)){Feedback=TEXT("Work orders are issued at ARRAY STATUS (left-hand PC), on its MAINTENANCE tab.");Changed(Feedback);UE_LOG(LogTemp,Warning,TEXT("S47 service rejected: not at the array PC"));return false;}
 if(!RequirePower())return false;
 if(!Field->HasArrayFault()&&!Field->TriggerServiceFault(ServiceGuide==0)){Feedback=TEXT("Service target unavailable. Restore station power and try again.");Changed(Feedback);return false;}
 GuideServiceBaseline=Field->ServiceCompletions();GuideSupplyBaseline=Field->ServiceSupplyDeliveries();
 ServiceGuide=1;bGuideSupply=bSupplyTrip;bGuideFocus=true;FieldFocus=-1;
 Feedback=FString::Printf(TEXT("WORK ORDER ISSUED / %s. At its cabinet outside: open the hatch and switch it to LOCAL, fit one relay, then close it and run the movement test. It stays off remote until the test passes; sign off here on MAINTENANCE after that."),*Field->ServiceDishId());Changed(Feedback);
 return true;
}
FString FS47FreeformSession::ServiceObjective()const
{
 if(ServiceGuide!=1||!Field)return FString();
 const int32 S=Field->ServiceStage();
 const auto At=[this](const TCHAR* Text){FString Result=FString(Text).Replace(TEXT("S-03"),*Field->ServiceDishId());return Field->ServiceDishId()==TEXT("S-03")?Result:Result.Replace(TEXT("CABLE JUNCTION"),TEXT("AZ / EL DRIVE"));};
 if(S==1)return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: INSPECT, ISOLATE AND OPEN THE RELAY HATCH"));
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)
 {
  // The receiving table is inside the garage: with the roll-up door down, that comes first.
  const bool bAtSaro=M&&M->Web&&M->Web->Area()==TEXT("saro");
  if(bAtSaro&&(Field->ParcelStage()==2||Field->ParcelStage()==3)&&M->EastWing&&M->EastWing->GarageShut())
   return TEXT("EAST-WING GARAGE: THE ROLL-UP DOOR IS DOWN. PRESS THE GREEN UP BUTTON BY THE DOOR (OR USE THE HAND CHAIN), THEN TAKE THE PARCEL TO THE RECEIVING TABLE");
  switch(Field->ParcelStage())
  {
   case 1:return TEXT("CALICHE GAS STATION: COLLECT THE PAID PARCEL");
   case 2:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("GARAGE RECEIVING: CARRY THE PARCEL TO THE LABELLED TABLE INSIDE THE EAST-WING GARAGE"):TEXT("TRUCK CARGO: HOLD THE PARCEL AND USE THE PARKED TRUCK TO LOAD IT");
   case 3:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("GARAGE RECEIVING: UNLOAD THE TRUCK, THEN CARRY THE PARCEL TO THE RECEIVING TABLE"):TEXT("TRUCK: DRIVE NORTH TO SARO AND UNLOAD AT THE EAST-WING GARAGE");
   case 4:return TEXT("PARCEL DROP SITE: RETRIEVE THE PARCEL WHERE YOU SET IT DOWN");
   default:return TEXT("CALICHE GAS STATION: DRIVE SOUTH; ENTER THE STORE AND BUY A RELAY OR DONOR AT THE COUNTER");
  }
 }
 if(Field->BenchBusy())return TEXT("EAST WING CALIBRATION BENCH: WAIT FOR THE AUTOMATIC CONTROLLER TEST; THE RESULT AND HARVESTED RELAY ARE RETAINED");
 if(S>=2&&S<=4&&Field->InventoryCount(TEXT("relay"))<=0)
  return Field->InventoryCount(TEXT("donor"))>0?TEXT("EAST WING CALIBRATION BENCH: TEST THE DONOR AND HARVEST ITS COMPATIBLE RELAY"):TEXT("PARTS STORAGE: CHECK RELAY STOCK; ENTER CALICHE'S STORE AND BUY A REPLACEMENT AT THE COUNTER IF EMPTY");
 switch(S)
 {
  case 2:return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: ISOLATE REMOTE MOVEMENT AND OPEN THE RELAY HATCH"));
  case 3:return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: OPEN THE RELAY COVER"));
  case 4:return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: SEAT THE COMPATIBLE REPLACEMENT RELAY"));
  case 5:return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: CLOSE THE RELAY COVER AND RUN THE MOVEMENT TEST"));
  case 6:return At(TEXT("S-03 CABLE JUNCTION / LOCAL BOX: START THE FIVE-DEGREE OUT-AND-BACK TEST"));
  case 7:case 8:return At(TEXT("S-03 LOCAL BOX: WATCH THE AUTOMATIC OUT-AND-BACK TEST; WHEN COMPLETE, SIGN OFF THE WORK ORDER AT ARRAY STATUS (LEFT-HAND PC)"));
  // D45-20261110: the work order is signed off on the array PC's MAINTENANCE tab
  case 9:return TEXT("ARRAY STATUS (left-hand PC) / MAINTENANCE: CHECK REMOTE IS BACK AND SIGN OFF THE WORK ORDER");
  default:return TEXT("ARRAY STATUS (left-hand PC) / MAINTENANCE: SEE THE OPEN WORK ORDER");
 }
}
bool FS47FreeformSession::CanCompleteServiceGuide() const
{
 if(ServiceGuide!=1||!Field||!M||!M->Web||Field->ServiceStage()!=9||Field->ServiceCompletions()<=GuideServiceBaseline)return false;
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)return false;
 const int32 Dish=Field->ServiceDishIndex();return Dish!=INDEX_NONE&&M->Web->DishRemoteAvailable(Dish);
}
bool FS47FreeformSession::CompleteServiceGuide()
{
 if(ServiceGuide!=1||!Field||!(AtServiceDesk(M)||AtOperations(M))||!RequirePower())return false;
 if(Field->ServiceStage()!=9||Field->ServiceCompletions()<=GuideServiceBaseline)
 {Feedback=ServiceObjective();Changed(Feedback);return false;}
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)
 {Feedback=TEXT("This route includes a new relay or donor transported in the truck and delivered at SARO. Existing stock alone does not complete that optional route.");Changed(Feedback);return false;}
 const int32 Dish=Field->ServiceDishIndex();
 if(Dish==INDEX_NONE||!M->Web->DishRemoteAvailable(Dish)){Feedback=TEXT("REMOTE is not restored. Inspect the target antenna at ARRAY STATUS.");Changed(Feedback);return false;}
 const FString Provenance=bGuideSupply?TEXT("A newly purchased relay/donor parcel was carried, secured in the truck and unpacked at SARO garage receiving during this work order."):TEXT("The local-stock route was selected; no supply trip is claimed.");
 RecordEvidence(HasFinding(TEXT("service-work-order"))?FString::Printf(TEXT("service-work-order-%d-%s"),Field->ServiceCompletions(),*Field->ServiceDishId()):FString(TEXT("service-work-order")),FString::Printf(TEXT("%s planned relay service / signed at %s.\n%s\nRelay isolated and exchanged with the cover secured. The real encoder passed the five-degree outward and return test. REMOTE restored; final AZ %.2f / EL %.2f. The original service record remains in the case archive. No observing-job payment was granted for this maintenance action."),*Field->ServiceDishId(),AtServiceDesk(M)?TEXT("ARRAY STATUS"):TEXT("OPS"),*Provenance,M->Web->DishAz(Dish),M->Web->DishEl(Dish)));
 ServiceGuide=2;bGuideFocus=false;Feedback=FString::Printf(TEXT("WORK ORDER SIGNED OFF / %s is back on remote. The record is filed under RECORDS at OPS."),*Field->ServiceDishId());Changed(Feedback);return true;
}
void FS47FreeformSession::OpenServiceGuide()
{
 // D45-20261110 (Tom: OPS can point to the array PC): a summary only. Faults, their order, work orders and sign-off are on
 // ARRAY STATUS (left-hand PC), MAINTENANCE.
 if(!bActive||!M||!M->UI||!RequirePower())return;
 TArray<S47Kit::FW> Rows;
 const FString Dish=Field?Field->ServiceDishId():FString(TEXT("S-03"));
 Rows.Add(S47FreeformTerminal::Paper(ServiceGuide==1?FString::Printf(TEXT("SARO / %s SERVICE WORK ORDER / IN PROGRESS"),*Dish):TEXT("SARO / ARRAY SERVICE WORK ORDERS")));
 Rows.Add(S47FreeformTerminal::Text(TEXT("ARRAY MAINTENANCE IS DONE AT THE ARRAY STATUS PC (LEFT-HAND). WORK ORDERS ARE ISSUED AND SIGNED THERE."),-1));
 if(!Feedback.IsEmpty())Rows.Add(S47FreeformTerminal::Text(Feedback,1));
 if(Field&&Field->ArrayWear().bActive)
 {
  // In the operator's order: the next one first, then the rest of the open ones, then those in service.
  const auto& Wear=Field->ArrayWear();TArray<S47FreeformTerminal::FRow> Lines;const auto* Next=Wear.NextBranch();
  TArray<const FS47ArrayWear::FBranch*> Order=Wear.Ordered();for(const auto& B:FS47ArrayWear::Branches())if(Wear.IsRestored(B.Id))Order.Add(&B);
  for(const auto* B:Order)
  {
   const bool Done=Wear.IsRestored(B->Id);const FString Missing=Done?FString():Field->BranchNeeds(*B);
   Lines.Add({{FString(B->Title),FString::Printf(TEXT("%d AT %s"),B->Dishes.Num(),B->Junction),Done?TEXT("IN SERVICE"):Missing.IsEmpty()?TEXT("PARTS READY"):TEXT("NEEDS PARTS")},B==Next,nullptr,B==Next?FString(TEXT("NEXT")):FString()});
  }
  Rows.Add(S47FreeformTerminal::Panel(FString::Printf(TEXT("STATION RESTORATION / %d OF 27"),27-Wear.DownCount()),S47FreeformTerminal::Table({TEXT("BRANCH"),TEXT("ANTENNAS"),TEXT("STATE")},{.42f,.26f,.32f},Lines)));
 }
 if(ServiceGuide==1)Rows.Add(S47FreeformTerminal::Paper(FString::Printf(TEXT("OPEN WORK ORDER / %s / %s"),*Dish,*ServiceObjective())));
 else if(ServiceGuide==2)Rows.Add(S47FreeformTerminal::Paper(TEXT("PREVIOUS WORK ORDER VERIFIED / ORIGINAL RETAINED IN RECORDS")));
 if(ServiceGuide!=1&&Field&&Field->HasArrayFault())Rows.Add(S47FreeformTerminal::Text(FString::Printf(TEXT("%s needs a new relay. Its work order is issued at the array PC."),*Dish)));
 Rows.Add(S47FreeformTerminal::Button(TEXT("SHOW ON THE ARRAY PC"),[this]{StatusTab=1;Feedback=TEXT("ARRAY STATUS (left-hand PC) is set to MAINTENANCE. Work orders are issued and signed there.");Changed(Feedback);OpenServiceGuide();},true));
 OpenTerminal(3,TEXT("OPS / SERVICE ORDER"),OpsShell(4,S47Kit::Col(Rows)));
}
