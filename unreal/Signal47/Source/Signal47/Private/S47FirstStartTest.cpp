#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47EastWing.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Mail.h"
#include "S47Props.h"
#include "S47RoomLights.h"
#include "S47Startup.h"
#include "S47Tape.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealClient.h"

// FIX-20261110 (Tom, points 4 and 8), -S47FirstStartTest (with -S47MailboxStart): a new operator at the box at the road. The
// station is locked and dark; the letter in the box has the keys in its envelope; inside the visitor entrance the walkie and the
// field recorder lie on the dresser, and every room's light is put on at its own switch.
void FS47FreeformTest::StepFirstStart(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>120){Finish(false,FString::Printf(TEXT("first start exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World||!M->EastWing||!M->RoomLights||!M->FieldRadio||!M->Freeform->Mail)return;
 auto& F=*M->Freeform;
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 auto Dark=[&]{bool B=true;for(const TCHAR* Room:{TEXT("control"),TEXT("corridor"),TEXT("records"),TEXT("office")})B&=!M->RoomLights->ManualEnabled(Room);return B;};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(F.bMailboxStart&&F.Mail->Holds(TEXT("ward:letter")),TEXT("a new operator stands at the box at the road with Dr. Ward's letter in it")))return;
  if(!Check(F.StationLocked(),TEXT("the station is locked while the keys are in the box")))return;
  if(!Check(Dark(),TEXT("every room's light is off at its switch")))return;
  if(!Check(F.Objective().Contains(TEXT("MAILBOX")),TEXT("the card on the HUD sends the operator to the mailbox; got ")+F.Objective()))return;
  Shot(TEXT("first-start-mailbox.png"));BoxX=M->Player->Pos.X;BoxZ=M->Player->Pos.Z;BoxYaw=M->Player->Yaw;Go(1,1.);return;
 }
 if(Stage==1)
 {
  if(!Check(bool(M->Player->OnUse),TEXT("the use callback is ready")))return;
  M->Player->Place(-7.4,5.7,-UE_DOUBLE_HALF_PI);M->Player->OnUse(TEXT("exitDoor"));
  if(!Check(!M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("the visitor entrance does not open without the keys")))return;
  M->Player->OnUse(TEXT("doorEast"));M->Player->OnUse(TEXT("labDoor"));M->Player->OnUse(TEXT("eastwing:garage-door-out"));M->Player->OnUse(TEXT("eastwing:personnel-door"));M->Player->OnUse(TEXT("eastwing:service-door"));
  const auto Wing=M->EastWing->Save();
  bool Shut=true;for(const auto& Door:M->Web->DoorStates())Shut&=!Door.Value;
  if(!Check(Shut&&!S47J::Bool(Wing,TEXT("personnel"),true)&&!S47J::Bool(Wing,TEXT("service"),true)&&!S47J::Bool(Wing,TEXT("garageUp"),true),TEXT("no other outside door or the garage opens either")))return;
  if(!Check(F.Save().IsValid()&&S47J::Bool(F.Save(),TEXT("stationLocked")),TEXT("a save made now keeps the station locked")))return;
  Go(2,.6);return;
 }
 if(Stage==2){Shot(TEXT("first-start-locked-door.png"));Go(3,.6);return;}
 if(Stage==3)
 {
  M->UI->Close(true);M->Player->Place(BoxX,BoxZ,BoxYaw);M->Player->OnUse(TEXT("ff:mail:box"));Go(4,1.);return;
 }
 if(Stage==4)
 {
  if(!Check(!F.Mail->Holds(TEXT("ward:letter"))&&!F.bStationLocked&&!F.StationLocked(),TEXT("the letter out of the box brings the keys: the station is unlocked")))return;
  if(!Check(!S47J::Bool(F.Save(),TEXT("stationLocked")),TEXT("and an unlocked station writes nothing to the save")))return;
  Shot(TEXT("first-start-letter.png"));Go(5,.8);return;
 }
 if(Stage==5)
 {
  M->UI->Close(true);M->Player->Place(-7.4,5.7,-UE_DOUBLE_HALF_PI);M->Player->OnUse(TEXT("exitDoor"));
  if(!Check(M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("with the keys the visitor entrance opens")))return;
  Go(6,1.2);return;
 }
 if(Stage==6){M->Player->TestMove=FVector2D(0,-1);Go(7);return;}
 if(Stage==7)
 {
  if(M->Player->Pos.X<-5.6)return;
  M->Player->TestMove.Reset();M->Player->Pitch=-.12;
  if(!Check(F.bDarkTold&&Dark(),TEXT("the first step inside is dark, and the operator is told where the switches are")))return;
  Go(8,1.);return;
 }
 if(Stage==8){Shot(TEXT("first-start-dark-hall.png"));Go(9,.6);return;}
 if(Stage==9){M->Player->Place(-3.25,5.95,0);M->Player->Pitch=-.24;Go(90,.5);return;} // at the corridor's switch, the eye on its plate
 if(Stage==90)
 {
  if(!Check(M->RoomLights->Use(TEXT("roomlight:corridor"))&&M->RoomLights->ManualEnabled(TEXT("corridor"))&&!M->RoomLights->ManualEnabled(TEXT("control")),TEXT("the corridor's switch puts on the corridor's light and no other")))return;
  Go(10,1.2);return;
 }
 if(Stage==10)
 {
  const bool bDresser=AS47Props::PlacedMesh(TEXT("hallDresser"))!=nullptr;
  if(bDresser)
  {
   const FVector R=M->FieldRadio->RootAt();const FBox B=AS47Props::PlacedMesh(TEXT("hallDresser"))->Bounds.GetBox();
   if(!Check(M->FieldRadio->InHall()&&F.Tape&&F.Tape->InHall(),TEXT("the walkie and the field recorder are on the hall dresser")))return;
   if(!Check(R.X*100>B.Min.X&&R.X*100<B.Max.X&&R.Z*100>B.Min.Y&&R.Z*100<B.Max.Y&&FMath::Abs(R.Y*100-B.Max.Z)<12,FString::Printf(TEXT("the walkie rests on the dresser's top, at %.2f %.2f %.2f (top %.2f)"),R.X,R.Y,R.Z,B.Max.Z/100)))return;
   if(!Check(B.Min.Y/100>4.78&&B.Max.Y/100<5.45&&B.Max.Z/100>.6&&B.Max.Z/100<.9,FString::Printf(TEXT("the dresser stands against the corridor's north wall, z %.2f..%.2f, top %.2f"),B.Min.Y/100,B.Max.Y/100,B.Max.Z/100)))return;
  }
  else Check(!M->FieldRadio->InHall()&&F.Tape&&!F.Tape->InHall(),TEXT("no dresser model in this build: the walkie and the recorder stay on their desks"));
  M->Player->Place(-6.3,5.75,-UE_DOUBLE_HALF_PI);M->Player->Pitch=-.12;Go(100,1.);return;
 }
 if(Stage==100){Shot(TEXT("first-start-hall-lit.png"));Go(11,.8);return;}
 if(Stage==11){M->Player->Place(-4.3,6.2,0);M->Player->Pitch=-.5;Go(12,1.2);return;}
 if(Stage==12){Shot(TEXT("first-start-dresser.png"));Go(13,.6);return;}
 if(Stage==13)
 {
  M->FieldRadio->Take();
  if(!Check(M->FieldRadio->Owned(),TEXT("the walkie is taken off the dresser")))return;
  Finish(true,TEXT("First start: locked and dark station, keys in the mailbox with the letter, walkie and recorder on the hall dresser, lights put on by hand."));return;
 }
}
