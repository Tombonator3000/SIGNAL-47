#include "S47Freeform.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Kit.h"

namespace
{
 bool AtOperations(US47Main* M)
 {
  if(!M||!M->Web||!M->Player||M->Web->Area()!=TEXT("saro")||!M->Web->Item(TEXT("opsTerminal")))return false;
  const FVector P=S47Three::ToThree(M->Web->PropLocation(TEXT("opsTerminal")));
  return FVector2D::Distance(FVector2D(P.X,P.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<3.5&&FMath::Abs(P.Y-M->Player->FloorY)<2.5;
 }
}
bool FS47FreeformSession::StartServiceGuide(bool bSupplyTrip)
{
 if(!bActive||ServiceGuide==1||!Field)return false;
 if(!AtOperations(M)){Feedback=TEXT("Use the OPS workstation inside SARO to accept service.");Changed(Feedback);UE_LOG(LogTemp,Warning,TEXT("S47 service rejected: not at OPS"));return false;}
 if(!RequirePower())return false;
 if(!Field->HasArrayFault()&&!Field->TriggerServiceFault(ServiceGuide==0)){Feedback=TEXT("Service target unavailable. Restore station power and try again.");Changed(Feedback);return false;}
 GuideServiceBaseline=Field->ServiceCompletions();GuideSupplyBaseline=Field->ServiceSupplyDeliveries();
 ServiceGuide=1;bGuideSupply=bSupplyTrip;bGuideFocus=true;FieldFocus=-1;
 Feedback=FString::Printf(TEXT("SERVICE ACCEPTED / %s. Go to its local cabinet outside: inspect / isolate, fit one relay, then close and test. ARRAY STATUS (left-hand PC) offers an optional diagnosis. Remote operation stays suspended until relay replacement and the movement test."),*Field->ServiceDishId());Changed(Feedback);
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
  switch(Field->ParcelStage())
  {
   case 1:return TEXT("CALICHE GAS STATION: COLLECT THE PAID PARCEL");
   case 2:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("GARAGE RECEIVING: CARRY THE PARCEL TO THE LABELLED TABLE INSIDE THE EAST-WING GARAGE"):TEXT("JEEP CARGO: HOLD THE PARCEL AND USE THE PARKED JEEP TO LOAD IT");
   case 3:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("GARAGE RECEIVING: UNLOAD THE JEEP, THEN CARRY THE PARCEL TO THE RECEIVING TABLE"):TEXT("JEEP: DRIVE NORTH TO SARO AND UNLOAD AT THE EAST-WING GARAGE");
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
  case 7:case 8:return At(TEXT("S-03 LOCAL BOX: WATCH THE AUTOMATIC OUT-AND-BACK TEST; WHEN COMPLETE, SIGN THE SERVICE ORDER AT OPS"));
  case 9:return TEXT("OPS (corner PC), SERVICE ORDER: VERIFY REMOTE RESTORATION AND ARCHIVE THE TEST");
  default:return TEXT("OPS (corner PC), SERVICE ORDER: INSPECT THE OPEN WORK ORDER");
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
 if(ServiceGuide!=1||!Field||!AtOperations(M)||!RequirePower())return false;
 if(Field->ServiceStage()!=9||Field->ServiceCompletions()<=GuideServiceBaseline)
 {Feedback=ServiceObjective();Changed(Feedback);return false;}
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)
 {Feedback=TEXT("This route includes a new relay or donor transported in the jeep and delivered at SARO. Existing stock alone does not complete that optional route.");Changed(Feedback);return false;}
 const int32 Dish=Field->ServiceDishIndex();
 if(Dish==INDEX_NONE||!M->Web->DishRemoteAvailable(Dish)){Feedback=TEXT("REMOTE is not restored. Inspect the target antenna at ARRAY STATUS.");Changed(Feedback);return false;}
 const FString Provenance=bGuideSupply?TEXT("A newly purchased relay/donor parcel was carried, secured in the service vehicle and unpacked at SARO garage receiving during this work order."):TEXT("The local-stock route was selected; no supply trip is claimed.");
 RecordEvidence(HasFinding(TEXT("service-work-order"))?FString::Printf(TEXT("service-work-order-%d-%s"),Field->ServiceCompletions(),*Field->ServiceDishId()):FString(TEXT("service-work-order")),FString::Printf(TEXT("%s planned relay service / signed at OPS.\n%s\nRelay isolated and exchanged with the cover secured. The real encoder passed the five-degree outward and return test. REMOTE restored; final AZ %.2f / EL %.2f. The original service record remains in the case archive. No observing-job payment was granted for this maintenance action."),*Field->ServiceDishId(),*Provenance,M->Web->DishAz(Dish),M->Web->DishEl(Dish)));
 ServiceGuide=2;bGuideFocus=false;Feedback=FString::Printf(TEXT("WORK ORDER ARCHIVED / %s is back in REMOTE. Select DAILY WORK or CASE ARCHIVE."),*Field->ServiceDishId());Changed(Feedback);return true;
}
void FS47FreeformSession::OpenServiceGuide()
{
 if(!bActive||!M||!M->UI||!RequirePower())return;
 TArray<S47Kit::FW> Rows;
 const FString Dish=Field?Field->ServiceDishId():FString(TEXT("S-03"));
 Rows.Add(S47FreeformTerminal::Paper(ServiceGuide==1?FString::Printf(TEXT("SARO / %s SERVICE WORK ORDER / IN PROGRESS"),*Dish):TEXT("SARO / ARRAY SERVICE WORK ORDERS")));
 if(!Feedback.IsEmpty())Rows.Add(S47FreeformTerminal::Text(Feedback,1));
 if(ServiceGuide!=1)
 {
  if(ServiceGuide==2)Rows.Add(S47FreeformTerminal::Paper(TEXT("PREVIOUS WORK ORDER VERIFIED / ORIGINAL RETAINED IN CASE ARCHIVE")));
  Rows.Add(S47FreeformTerminal::Text(Field&&Field->HasArrayFault()?FString::Printf(TEXT("%s needs service. Accept the existing fault below; other antennas and observing jobs remain available."),*Dish):ServiceGuide==0?TEXT("FIRST SERVICE / S-03. Learn to diagnose, replace a relay and run its real movement test. Accepting isolates S-03 from remote operation."):TEXT("Optional planned maintenance. The next order selects an antenna from the array; its identity is retained until repaired. Accepting isolates only that antenna.")));
  Rows.Add(S47FreeformTerminal::Text(TEXT("Use a relay from stock, or choose a new supply trip: buy a relay or donor in Caliche, carry its parcel in the jeep, then unpack at GARAGE RECEIVING. Donors must be tested at CALIBRATION / REPAIR.")));
  Rows.Add(S47FreeformTerminal::Button(TEXT("Accept / use SARO stock"),[this]{StartServiceGuide(false);OpenServiceGuide();},true));
  Rows.Add(S47FreeformTerminal::Button(TEXT("Accept / collect and transport new supplies"),[this]{StartServiceGuide(true);OpenServiceGuide();}));
 }
 else
 {
  Rows.Add(S47FreeformTerminal::Paper(ServiceObjective()));
  Rows.Add(S47FreeformTerminal::Text(FString::Printf(TEXT("REPAIR TARGET: %s. At its outside cabinet: LOCAL, cover, relay, cover, local movement test. Return here only after REMOTE is restored."),*Dish)));
  if(CanCompleteServiceGuide())Rows.Add(S47FreeformTerminal::Button(TEXT("Verify REMOTE and archive the completed work order"),[this]{if(CompleteServiceGuide())OpenArchive();else OpenServiceGuide();},true));
  else Rows.Add(S47FreeformTerminal::Button(TEXT("Continue service / return to room"),[this]{bGuideFocus=true;FieldFocus=-1;Feedback=ServiceObjective();Changed(Feedback);M->UI->Close(true);},true));
  Rows.Add(S47FreeformTerminal::Button(bGuideFocus?TEXT("Show observing objective / retain unfinished service"):TEXT("Follow service objective"),[this]{bGuideFocus=!bGuideFocus;if(bGuideFocus)FieldFocus=-1;Feedback=bGuideFocus?ServiceObjective():TEXT("OBSERVING FOCUS / unfinished service retained.");Changed(Feedback);OpenServiceGuide();}));
 }
 OpenTerminal(3,TEXT("OPS / SERVICE ORDER"),OpsShell(4,S47Kit::Col(Rows)));
}
