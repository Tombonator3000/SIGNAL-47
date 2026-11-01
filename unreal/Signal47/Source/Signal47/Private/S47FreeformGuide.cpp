#include "S47Freeform.h"
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
 if(!bActive||ServiceGuide!=0||!Field||!AtOperations(M)||!RequirePower())return false;
 if(!Field->HasArrayFault()&&!Field->TriggerServiceFault())return false;
 GuideServiceBaseline=Field->ServiceCompletions();GuideSupplyBaseline=Field->ServiceSupplyDeliveries();
 ServiceGuide=1;bGuideSupply=bSupplyTrip;bGuideFocus=true;
 Changed(TEXT("Planned S-03 service accepted. Remote operation is suspended until relay replacement and the actual movement test. Read the left status PC first."));
 return true;
}
FString FS47FreeformSession::ServiceObjective()const
{
 if(ServiceGuide!=1||!Field)return FString();
 const int32 S=Field->ServiceStage();
 if(S==1)return TEXT("SERVICE ORDER // SELECT S-03 ON THE LEFT STATUS PC");
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)
 {
  switch(Field->ParcelStage())
  {
   case 1:return TEXT("SUPPLIES // COLLECT THE PAID PARCEL AT CALICHE'S GAS STATION");
   case 2:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("SERVICE BAY // DELIVER THE PARCEL TO THE BENCH BY THE SARO SIGN"):TEXT("SUPPLIES // LOAD THE PARCEL IN THE JEEP CARGO RACK");
   case 3:return M&&M->Web&&M->Web->Area()==TEXT("saro")?TEXT("SERVICE BAY // UNLOAD THE JEEP, THEN CARRY THE PARCEL TO THE BENCH"):TEXT("RETURN TO SARO // DRIVE NORTH AND UNLOAD AT THE SERVICE BAY");
   case 4:return TEXT("SUPPLIES // RETRIEVE THE PARCEL WHERE YOU SET IT DOWN");
   default:return TEXT("CALICHE GAS STATION // DRIVE SOUTH; BUY A RELAY OR DONOR AT THE SUPPLY HATCH");
  }
 }
 if(Field->BenchBusy())return TEXT("SERVICE BENCH // WAIT FOR THE PHYSICAL CONTROLLER TEST");
 if(S>=2&&S<=4&&Field->InventoryCount(TEXT("relay"))<=0)
  return Field->InventoryCount(TEXT("donor"))>0?TEXT("SERVICE BENCH // TEST THE DONOR AND HARVEST ITS COMPATIBLE RELAY"):TEXT("REPLACEMENT RELAY // CHECK SARO STOCK OR CALICHE'S SUPPLY HATCH");
 switch(S)
 {
  case 2:return TEXT("S-03 SERVICE PANEL // SELECT LOCAL TO ISOLATE REMOTE MOVEMENT");
  case 3:return TEXT("S-03 SERVICE PANEL // OPEN THE RELAY COVER");
  case 4:return TEXT("S-03 SERVICE PANEL // SEAT THE COMPATIBLE REPLACEMENT RELAY");
  case 5:return TEXT("S-03 SERVICE PANEL // SECURE THE RELAY COVER");
  case 6:return TEXT("S-03 LOCAL CONTROL // START THE FIVE-DEGREE OUT-AND-BACK TEST");
  case 7:case 8:return TEXT("S-03 LOCAL TEST // WATCH THE DISH MOVE AND RETURN TO ITS STARTING ANGLE");
  case 9:return TEXT("OPS SERVICE ORDER // VERIFY REMOTE RESTORATION AND ARCHIVE THE TEST");
  default:return TEXT("OPS SERVICE ORDER // INSPECT THE OPEN WORK ORDER");
 }
}
bool FS47FreeformSession::CompleteServiceGuide()
{
 if(ServiceGuide!=1||!Field||!AtOperations(M)||!RequirePower())return false;
 if(Field->ServiceStage()!=9||Field->ServiceCompletions()<=GuideServiceBaseline)
 {Changed(TEXT("Complete the actual local movement test before signing this work order."));return false;}
 if(bGuideSupply&&Field->ServiceSupplyDeliveries()<=GuideSupplyBaseline)
 {Changed(TEXT("This route includes a new relay or donor transported in the jeep and delivered at SARO. Existing stock alone does not complete that optional route."));return false;}
 int32 Dish=INDEX_NONE;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==TEXT("S-03")){Dish=I;break;}
 if(Dish==INDEX_NONE||!M->Web->DishRemoteAvailable(Dish))return false;
 const FString Provenance=bGuideSupply?TEXT("A newly purchased relay/donor parcel was carried, secured in the service vehicle and delivered to the SARO bench during this work order."):TEXT("The local-stock route was selected; no supply trip is claimed.");
 RecordEvidence(TEXT("service-work-order"),FString::Printf(TEXT("S-03 planned relay service / signed at OPS.\n%s\nRelay isolated and exchanged with the cover secured. The real encoder passed the five-degree outward and return test. REMOTE restored; final AZ %.2f / EL %.2f. The original service record remains in the case archive. No observing-job payment was granted for this maintenance action."),*Provenance,M->Web->DishAz(Dish),M->Web->DishEl(Dish)));
 ServiceGuide=2;bGuideFocus=false;Changed(TEXT("Service work order verified and archived. S-03 is back in REMOTE. Continue daily observing or inspect the case archive."));return true;
}
void FS47FreeformSession::OpenServiceGuide()
{
 if(!bActive||!M||!M->UI||!RequirePower())return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(TEXT("SARO / S-03 SERVICE WORK ORDER\nOptional planned maintenance: diagnose, replace, physically test and retain the result. S-03 is taken out of remote operation when you accept. Other antennas and daily jobs remain available.")));
 if(ServiceGuide==0)
 {
  Rows.Add(S47Kit::Text(TEXT("Use the relay already held at SARO, or practise the supply route: drive south to Caliche's gas station, buy a relay ($25) or donor ($15), collect and load its parcel, return north and unload at the bench beside the SARO sign. Donors require a bench test. The notebook map retains this route.")));
  Rows.Add(S47Kit::Button(TEXT("Accept / use SARO stock"),[this]{if(StartServiceGuide(false))M->UI->Close();},true));
  Rows.Add(S47Kit::Button(TEXT("Accept / collect and transport new supplies"),[this]{if(StartServiceGuide(true))M->UI->Close();}));
 }
 else if(ServiceGuide==1)
 {
  Rows.Add(S47Kit::Paper(ServiceObjective()));
  Rows.Add(S47Kit::Text(TEXT("Left PC: select S-03. Outside at S-03: LOCAL, cover, relay, cover, local test. Supply hatch opens 08:00–20:00; essential relay requisition remains available after hours. Looking at an existing stock count does not certify a new supply trip.")));
  Rows.Add(S47Kit::Button(TEXT("Verify REMOTE and archive the completed work order"),[this]{if(CompleteServiceGuide())OpenArchive();else OpenServiceGuide();},true));
  Rows.Add(S47Kit::Button(bGuideFocus?TEXT("Show observing objective / retain unfinished service"):TEXT("Follow service objective"),[this]{bGuideFocus=!bGuideFocus;Changed();M->UI->Close();}));
 }
 else Rows.Add(S47Kit::Paper(TEXT("WORK ORDER VERIFIED / ORIGINAL RETAINED\nS-03 completed its actual local movement test and returned to REMOTE. The case archive retains the signed service record.")));
 SetPanel(S47Kit::Shell(M->UI,TEXT("SARO / SERVICE ORDER"),S47Kit::Col(Rows),960));
}
