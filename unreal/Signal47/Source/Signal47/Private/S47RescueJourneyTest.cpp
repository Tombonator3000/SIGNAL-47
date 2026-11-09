#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47EastWing.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47HUD.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Truck.h"
#include "S47Drive.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Props.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 bool RescueButtonHas(const TSharedRef<SWidget>& Node,const FString& Label)
 {
  if(Node->GetTypeAsString()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(Node)->GetText().ToString().Contains(Label))return true;
  FChildren* Kids=Node->GetChildren();
  for(int32 I=0;Kids&&I<Kids->Num();++I)if(RescueButtonHas(Kids->GetChildAt(I),Label))return true;
  return false;
 }
 void RescueButtons(const TSharedRef<SWidget>& Node,const FString& Label,TArray<TSharedRef<SButton>>& Out)
 {
  if(Node->GetTypeAsString()==TEXT("SButton")&&RescueButtonHas(Node,Label))Out.Add(StaticCastSharedRef<SButton>(Node));
  FChildren* Kids=Node->GetChildren();
  for(int32 I=0;Kids&&I<Kids->Num();++I)RescueButtons(Kids->GetChildAt(I),Label,Out);
 }
}

// GAMEPLAY-03: from an empty account, debt, an outage and a shut garage back to paid work.
// Run: -S47FreeformTest -S47RescueJourneyTest -UserDir=/tmp/s47-rescue
// Declared fixtures: the starting state and the near-empty tank are saved-state patches, and placement
// replaces the long drives and walks. Money, parts, fuel and power then change only through production
// callbacks: door control, truck, counter buttons, parcel, rack, roadside, receiving, feeder, recording, pump.
void FS47FreeformTest::StepRescueJourney(double Dt)
{
 using EJ=FS47FreeformSession::EJobStage;
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>420){Finish(false,FString::Printf(TEXT("rescue journey exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter||!M->World||!M->EastWing)return;
 auto& F=*M->Freeform;if(!F.Field||!F.Director)return;auto& W=*F.Field;auto* Wd=M->World.Get();
 auto Garage=[&]{return S47J::Num(M->EastWing->Save(),TEXT("garage"),1.);};
 auto AimAt=[&](const FVector& Target)
 {
  const FVector Gap=Target-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  M->Player->Yaw=FMath::Atan2(-Gap.X,-Gap.Z);M->Player->Pitch=FMath::Atan2(Gap.Y,FVector2D(Gap.X,Gap.Z).Size());
 };
 auto Click=[&](const FString& Label)
 {
  TArray<TSharedRef<SButton>> Found;
  if(auto Modal=M->Hud?M->Hud->ModalWidget():nullptr)RescueButtons(Modal.ToSharedRef(),Label,Found);
  if(!Check(Found.Num()==1&&Found[0]->IsEnabled(),TEXT("unique enabled native button: ")+Label))return false;
#if !UE_BUILD_SHIPPING
  Found[0]->SimulateClick();return true;
#else
  Finish(false,TEXT("native rescue journey requires a development build"));return false;
#endif
 };

 // ---- Stranded: no money, no parts, no power, the Jeep behind a dead opener ----
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);F.Director->Stop();
  if(!Check(M->Flow->bDevRun&&W.ArrayWear().bActive&&F.AvailableDishCount()==12,TEXT("isolated new game on the worn twelve-antenna station")))return;
  if(!Check(Garage()==0.,TEXT("new game's garage door is shut with the Jeep inside")))return;
  RestorePatch=[](const S47J::FObj& State)
  {
   const auto Session=S47J::Obj(State,TEXT("freeform"));const auto Supply=S47J::Obj(Session,TEXT("field"));const auto Parts=S47J::Obj(Supply,TEXT("stock"));
   if(!Parts)return;
   S47J::Set(Session,TEXT("cash"),0);S47J::Set(Session,TEXT("loan"),10);S47J::Set(Session,TEXT("powerFault"),true);S47J::Set(Session,TEXT("clock"),22.*3600.);
   S47J::Set(Supply,TEXT("fuel"),1.);
   for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("fuel"),TEXT("donor"),TEXT("film")})S47J::Set(Parts,Id,0);
  };
  Check(true,TEXT("DECLARED fixture: $0, $10 standing debt, active outage, no spares, 22:00, one tank unit"));
  if(!Restore(TEXT("rescue-stranded")))return;Go(1,.6);return;
 }
 if(Stage==1)
 {
  F.Director->Stop();M->UI->Close(true);
  if(!Check(F.Budget()==0&&F.Debt()==10&&F.PowerFault()&&!F.HasPowerForReactiveLight()&&FMath::IsNearlyEqual(W.VehicleFuel(),1.),TEXT("fixture state survives disk and Flow: no money, debt, no power")))return;
  if(!Check(W.InventoryCount(TEXT("fuse"))==0&&W.InventoryCount(TEXT("relay"))==0&&W.InventoryCount(TEXT("fuel"))==0,TEXT("SARO stock holds no fuse, relay or fuel can")))return;
  if(!Check(Garage()==0.&&!W.ShopOpen(),TEXT("garage still shut, and the shop is closed at this hour")))return;
  if(!Check(!F.Spend(5,true)&&F.Budget()==0&&F.Debt()==10,TEXT("standing debt refuses ordinary emergency credit without changing the balance")))return;
  const FVector Selector=W.InteractionPoint(TEXT("ff:generator"));M->Player->Place(Selector.X-1.2,Selector.Z,0);W.Use(TEXT("ff:generator"));
  if(!Check(!W.GeneratorRunning(),TEXT("backup generator cannot start without a fuel can")))return;
  F.Use(TEXT("opsTerminal"));
  if(!Check(!M->UI->Modal(),TEXT("unpowered OPS opens no work, so no money can be earned yet")))return;
  if(!Check(F.Objective().Contains(TEXT("CALICHE STORE COUNTER")),TEXT("objective names the way out: ")+F.Objective()))return;
  Go(2,.4);return;
 }
 if(Stage==2)
 {
  auto* Door=M->Inter->Get(TEXT("eastwing:garage-door-in"));
  if(!Check(Door&&Door->Label&&Door->Label().Contains(TEXT("hand chain")),TEXT("dead opener offers its hand chain in the interaction label")))return;
  if(!Check(bool(M->Player->OnUse),TEXT("door control callback ready")))return;
  M->Player->OnUse(TEXT("eastwing:garage-door-in"));Go(3);return;
 }
 if(Stage==3)
 {
  if(Garage()<.95)return;
  if(!Check(F.PowerFault()&&!F.HasPowerForReactiveLight(),TEXT("hand chain raised the door with the station still dark")))return;
  const auto Choice=Wd->TruckChoice(TEXT("saro"));
  if(!Check(Choice.Label.Contains(TEXT("Drive"))&&bool(Choice.Use),TEXT("the fuelled Jeep offers an ordinary drive")))return;
  Choice.Use();Go(4);return;
 }
 if(Stage==4)
 {
  if(Wd->bBusy||!Wd->bDriving||!Wd->CabDrive())return;
  Wd->TestInput=[]{FS47DriveIn In;In.Throttle=.6;return In;};Go(5);return;
 }
 if(Stage==5)
 {
  const auto* Cab=Wd->CabDrive();if(!Cab||Cab->Pos.Z<11)return;
  Wd->TestInput=nullptr;
  if(!Check(W.VehicleFuel()<1.&&W.VehicleFuel()>.9,FString::Printf(TEXT("Jeep drives out under the hand-raised door and burns its own fuel, at z %.2f"),Cab->Pos.Z)))return;
  Check(true,TEXT("DECLARED fixture: placement replaces the drive to Caliche"));
  Wd->StopDriving();Wd->Enter(TEXT("road"));Go(6,.6);return;
 }

 // ---- Caliche after hours: the credit fuse needs no money ----
 if(Stage==6)
 {
  if(Wd->bBusy)return;Wd->PlaceTruck(TEXT("diner"));Wd->Enter(TEXT("diner"));
  M->Player->Place(7983.75,2067.2005,0);if(!AS47Props::IsDoorOpen(TEXT("gasDoor")))M->Inter->UseId(TEXT("town:gasDoor"));
  Go(7,.6);return;
 }
 if(Stage==7)
 {
  if(Wd->bBusy)return;
  if(!Check(AS47Props::IsDoorOpen(TEXT("gasDoor")),TEXT("shop door opens for a player after hours")))return;
  M->Player->Place(7981.8,2068.8,0);AimAt(W.InteractionPoint(TEXT("ff:shop")));Go(8,.4);return;
 }
 if(Stage==8)
 {
  AimAt(W.InteractionPoint(TEXT("ff:shop")));
  if(!Check(!W.ShopOpen()&&!W.Purchase(TEXT("relay"))&&W.ParcelStage()==0&&F.Budget()==0&&F.Debt()==10,TEXT("closed counter refuses an ordinary sale and charges nothing")))return;
  Go(9,.3);return;
 }
 if(Stage==9)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("ff:shop")&&bool(M->Player->OnUse),TEXT("actual eye-ray reaches the counter; got ")+Pick))return;
  M->Player->OnUse(TEXT("ff:shop"));
  if(!Check(M->UI->Modal(),TEXT("counter opens its menu after hours")))return;
  Go(10,.3);return;
 }
 if(Stage==10)
 {
  if(!Click(TEXT("Request feeder-repair fuse")))return;
  if(!Check(W.ParcelStage()==1&&W.ParcelContents().FindRef(TEXT("fuse"))==1&&F.Budget()==0&&F.Debt()==15&&W.InventoryCount(TEXT("fuse"))==0,TEXT("credit fuse issued with debt and no money: a physical parcel, $5 more debt, nothing in stock yet")))return;
  if(!Check(!W.EmergencyFuseAvailable()&&!W.RequestEmergencyFuse()&&F.Debt()==15,TEXT("a second credit fuse for the same outage is refused")))return;
  M->UI->Close(true);AimAt(W.InteractionPoint(TEXT("ff:parcel")));Go(11,.4);return;
 }
 if(Stage==11)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("ff:parcel"),TEXT("actual eye-ray reaches the counter parcel; got ")+Pick))return;
  M->Player->OnUse(TEXT("ff:parcel"));
  if(!Check(W.ParcelStage()==2,TEXT("parcel taken in both hands")))return;
  const FVector Rack=W.InteractionPoint(TEXT("ff:cargo"));M->Player->Place(Rack.X+1.4,Rack.Z,0);Go(12,.4);return;
 }
 if(Stage==12)
 {
  const auto Choice=Wd->TruckChoice(M->Web->Area());
  if(!Check(Choice.Label==TEXT("Load parcel into vehicle")&&bool(Choice.Use),TEXT("parked Jeep offers to load the held parcel")))return;
  Choice.Use();
  if(!Check(W.ParcelStage()==3&&!Wd->bDriving,TEXT("parcel secured in the rack")))return;
  RestorePatch=[](const S47J::FObj& State){if(const auto Supply=S47J::Obj(S47J::Obj(State,TEXT("freeform")),TEXT("field")))S47J::Set(Supply,TEXT("fuel"),.0012);};
  Check(true,TEXT("DECLARED fixture: the tank is nearly out when leaving Caliche"));
  if(!Restore(TEXT("rescue-low-tank")))return;Go(13,.6);return;
 }

 // ---- The tank dies with the fuse on the rack ----
 if(Stage==13)
 {
  F.Director->Stop();M->UI->Close(true);
  if(!Check(W.ParcelStage()==3&&W.HasVehicleFuel(),TEXT("loaded parcel and low tank survive disk and Flow")))return;
  const auto Choice=Wd->TruckChoice(M->Web->Area());
  if(!Check(Choice.Label.Contains(TEXT("Drive"))&&bool(Choice.Use),TEXT("a tank with fuel left still offers the drive")))return;
  Choice.Use();Go(14);return;
 }
 if(Stage==14)
 {
  if(Wd->bBusy||!Wd->bDriving||!Wd->CabDrive())return;
  Wd->TestInput=[]{FS47DriveIn In;In.Throttle=.6;return In;};Go(15);return;
 }
 if(Stage==15)
 {
  if(W.HasVehicleFuel())return;
  Wd->TestInput=nullptr;
  if(!Check(F.Objective().Contains(TEXT("TANK EMPTY")),TEXT("real driving empties the tank and the objective says what to do: ")+F.Objective()))return;
  Go(16,1.);return;
 }
 if(Stage==16)
 {
  if(!Check(FMath::Abs(Wd->DrivingSpeed())<=.6&&Wd->GetOutHere(),TEXT("the dead Jeep comes to rest and the driver can get out")))return;
  Go(17,.8);return;
 }
 if(Stage==17)
 {
  if(Wd->bBusy||Wd->bDriving)return;M->UI->Close(true);
  const FVector Rack=W.InteractionPoint(TEXT("ff:cargo"));M->Player->Place(Rack.X+1.4,Rack.Z,0);
  if(!Check(W.Label(TEXT("ff:cargo")).Contains(TEXT("roadside")),TEXT("the rack itself names roadside return with a parcel loaded")))return;
  const auto Choice=Wd->TruckChoice(M->Web->Area());
  if(!Check(Choice.Label.Contains(TEXT("Tank empty"))&&bool(Choice.Use),TEXT("the empty Jeep offers help instead of a drive that cannot move")))return;
  Choice.Use();
  if(!Check(M->UI->Modal()&&W.ParcelStage()==3,TEXT("a loaded rack on an empty tank opens the menu and leaves the parcel secured")))return;
  Go(18,.3);return;
 }
 if(Stage==18)
 {
  RescueClock=F.Clock();
  if(!Click(TEXT("Request roadside return")))return;
  if(!Check(M->Web->Area()==TEXT("saro")&&!Wd->bDriving&&FMath::IsNearlyEqual(W.VehicleFuel(),2.)&&W.ParcelStage()==3&&W.ParcelContents().FindRef(TEXT("fuse"))==1,TEXT("roadside brings truck and loaded fuse to SARO with a two-unit reserve")))return;
  if(!Check(F.Budget()==0&&F.Debt()==15&&F.Clock()>=RescueClock+900&&F.HasFinding(TEXT("roadside"))&&F.Arrears()==20,TEXT("roadside on an empty account goes on the weekly sheet, takes time, and is on record")))return;
  // TOW-SHOW-20261109 (Tom): towed home, the truck stands on its pad and is drawn, not just its handle and the parcel on its rack.
  if(!Check(Wd->TruckAt==TEXT("saro")&&Wd->SaroTruck()&&Wd->SaroTruck()->Group.Shown()&&Wd->TruckHere()==Wd->SaroTruck()&&!(Wd->CabTruck()),TEXT("towed home, the truck is on its pad at SARO and can be seen")))return;
  Go(19,.8);return;
 }

 // ---- Home: unpack, repair the feeder ----
 if(Stage==19)
 {
  M->UI->Close(true);
  if(!Check(M->Player->Walkable(M->Player->Pos.X,M->Player->Pos.Z),TEXT("roadside leaves the player on walkable SARO ground")))return;
  const FVector Rack=W.InteractionPoint(TEXT("ff:cargo"));M->Player->Place(Rack.X+1.4,Rack.Z,0);W.Use(TEXT("ff:cargo"));
  if(!Check(W.ParcelStage()==2,TEXT("with fuel in the tank the rack unloads as usual")))return;
  const FVector Table=W.InteractionPoint(TEXT("ff:delivery"));M->Player->Place(Table.X-1.4,Table.Z,0);Go(20,.4);return;
 }
 if(Stage==20)
 {
  W.Use(TEXT("ff:delivery"));
  if(!Check(W.ParcelStage()==0&&W.InventoryCount(TEXT("fuse"))==1&&F.Debt()==15,TEXT("fuse delivered to SARO stock exactly once")))return;
  Check(true,TEXT("DECLARED fixture: placement replaces the drive to the feeder"));
  Wd->Enter(TEXT("road"));Go(21,.6);return;
 }
 if(Stage==21)
 {
  if(Wd->bBusy)return;
  const FVector Panel=W.InteractionPoint(TEXT("ff:feeder"));M->Player->Place(Panel.X-1.4,Panel.Z,0);Go(22,.6);return;
 }
 if(Stage==22)
 {
  for(int32 Press=0;Press<4;++Press)W.Use(TEXT("ff:feeder"));
  if(!Check(W.FeederStage()==4&&W.InventoryCount(TEXT("fuse"))==0,TEXT("measure, isolate, seat the one fuse and start the timed test")))return;
  Go(23);return;
 }
 if(Stage==23)
 {
  if(F.PowerFault())return;
  if(!Check(F.HasPowerForReactiveLight()&&F.HasFinding(TEXT("feeder-repair"))&&F.Budget()==0&&F.Debt()==15,TEXT("timed test restores mains; no money was needed")))return;
  if(!Restore(TEXT("rescue-mains")))return;M->World->Enter(TEXT("saro"));M->Player->Place(0,-1.2,PI);Go(24,.6);return;
 }

 // ---- Paid work again ----
 if(Stage==24)
 {
  F.Director->Stop();M->UI->Close(true);
  if(!Check(!F.PowerFault()&&F.HasPowerForReactiveLight(),TEXT("repaired feeder survives disk and Flow")))return;
  int32 Pending=INDEX_NONE;for(int32 I=0;I<F.Jobs().Num();++I){const auto& Job=F.Jobs()[I];if(Job.Day==F.Day()&&Job.Stage==EJ::New&&Job.Source!=0){Pending=I;break;}}
  if(!Check(Pending!=INDEX_NONE&&F.SelectJob(Pending),TEXT("an unfinished paid observation is still waiting")))return;
  const auto* P=F.ConsoleProfile();if(!Check(P&&M->Story,TEXT("observation has a work card")))return;
  Check(true,TEXT("DECLARED fixture: receiver dials are set from the work card"));
  auto& Rx=M->Story->Rx;Rx.Frequency=P->F;Rx.Azimuth=(P->Az[0]+P->Az[1])/2;Rx.Gain=(P->Gain[0]+P->Gain[1])/2;Rx.Bandwidth=(P->Bw[0]+P->Bw[1])/2;
  if(!Check(F.PointSelected(),TEXT("array commanded on the worn station")))return;
  Go(25);return;
 }
 if(Stage==25){if(!F.StartAcquisition())return;Go(26);return;}
 if(Stage==26)
 {
  const auto* Current=F.CurrentJob();if(!Current||Current->Stage!=EJ::Chart||F.PrinterProgress()<1)return;
  if(!Check(F.CollectChart()&&F.CurrentJob()->Stage==EJ::Recorded&&!F.CurrentJob()->bAnomaly,TEXT("timed recording and printer retain a routine original")))return;
  if(!Check(F.RegisterComparison(TEXT("normal"))&&F.Debt()==0&&F.Budget()==0&&F.Arrears()==15,TEXT("registered work pays $20: the $15 debt first, then $5 off the weekly sheet")))return;
  RestorePatch=[](const S47J::FObj& State)
  {
   const auto Session=S47J::Obj(State,TEXT("freeform"));const auto Supply=S47J::Obj(Session,TEXT("field"));
   if(!Supply)return;
   S47J::Set(Session,TEXT("cash"),2);S47J::Set(Supply,TEXT("fuel"),.5);
  };
  Check(true,TEXT("DECLARED fixture: $2 left and half a tank unit, below the old $4 pump gate"));
  if(!Restore(TEXT("rescue-last-dollars")))return;Go(27,.6);return;
 }

 // ---- The last dollars still buy fuel ----
 if(Stage==27)
 {
  F.Director->Stop();M->UI->Close(true);
  if(!Check(F.Budget()==2&&F.Debt()==0&&FMath::IsNearlyEqual(W.VehicleFuel(),.5),TEXT("settled balance and fixture tank survive disk and Flow")))return;
  Wd->Enter(TEXT("road"));Go(28,.6);return;
 }
 if(Stage==28)
 {
  if(Wd->bBusy)return;Wd->PlaceTruck(TEXT("diner"));Wd->Enter(TEXT("diner"));Go(29,.6);return;
 }
 if(Stage==29)
 {
  if(Wd->bBusy)return;
  const FVector Nozzle=W.InteractionPoint(TEXT("ff:pump"));
  M->Player->Place(Nozzle.X+2.5,Nozzle.Z,0);const double Floor=M->Player->FloorY;
  auto Pose=S47J::New();S47J::Set(Pose,TEXT("at"),FString(TEXT("diner")));S47J::Set(Pose,TEXT("x"),Nozzle.X+2.5);S47J::Set(Pose,TEXT("y"),Floor);S47J::Set(Pose,TEXT("z"),Nozzle.Z);S47J::Set(Pose,TEXT("h"),0.);S47J::Set(Pose,TEXT("p"),0.);S47J::Set(Pose,TEXT("r"),0.);
  Wd->PlaceTruck(TEXT("diner"),Pose);M->Player->Place(Nozzle.X+1.2,Nozzle.Z,0);Go(30,.6);return;
 }
 if(Stage==30)
 {
  if(!Check(W.StartPump()&&W.PumpActive()&&F.Budget()==2,TEXT("pump starts on $2 although a full metered fill would cost $4")))return;
  Go(31);return;
 }
 if(Stage==31)
 {
  if(W.PumpActive())return;
  if(!Check(W.PumpCharges()==2&&F.Budget()==0&&FMath::IsNearlyEqual(W.VehicleFuel(),1.),TEXT("pump delivers what $2 buys, charges only that and stops when the budget is gone")))return;
  Finish(true,TEXT("Empty account, debt, outage and shut garage back to paid work: hand chain, drive out, after-hours credit fuse, tank dying with the fuse on the rack, roadside return, receiving, feeder repair, one paid registration, last dollars at the pump. Declared state and placement fixtures; no long drive or human pacing."));return;
 }
}
