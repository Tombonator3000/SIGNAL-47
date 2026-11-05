#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47EastWing.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Truck.h"
#include "S47Drive.h"
#include "S47Startup.h"
#include "UnrealClient.h"

// A new Freeform game: on foot at the visitor entrance, doors shut, Jeep in the closed garage.
void FS47FreeformTest::StepGarageStart(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>120){Finish(false,FString::Printf(TEXT("garage start exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World||!M->EastWing)return;
 auto* W=M->World.Get();const auto* D=W->CabDrive();
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 auto Garage=[&]{return S47J::Num(M->EastWing->Save(),TEXT("garage"),1.);};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  const auto P=M->Player->Pos;
  if(!Check(FMath::Abs(P.X+9.5)<.3&&FMath::Abs(P.Z-5.7)<.3,FString::Printf(TEXT("new game starts on foot on the visitor ramp, at %.2f %.2f"),P.X,P.Z)))return;
  bool Shut=true;for(const auto& Door:M->Web->DoorStates())Shut&=!Door.Value;
  const auto Wing=M->EastWing->Save();
  if(!Check(Shut&&!S47J::Bool(Wing,TEXT("personnel"),true)&&!S47J::Bool(Wing,TEXT("service"),true),TEXT("every station and east wing door starts shut")))return;
  if(!Check(Garage()==0.&&!S47J::Bool(Wing,TEXT("garageUp"),true),TEXT("garage door starts fully down")))return;
  const auto Pose=S47J::Obj(W->TruckSave(),TEXT("pose"));
  if(!Check(W->TruckAt==TEXT("saro")&&Pose&&FMath::Abs(S47J::Num(Pose,TEXT("x"))-35.4)<.1&&FMath::Abs(S47J::Num(Pose,TEXT("z"))+.4)<.1&&FMath::Abs(S47J::Num(Pose,TEXT("y")))<.08,TEXT("Jeep stands level in the middle of the garage")))return;
  Shot(TEXT("garage-start-view.png"));Go(1,1.);return;
 }
 if(Stage==1){M->Player->TestMove=FVector2D(0,-1);Go(2);return;}
 if(Stage==2)
 {
  if(M->Player->Pos.X<-7.6)return;
  M->Player->TestMove.Reset();
  if(!Check(M->Web->ToggleDoor(TEXT("exitDoor"))&&M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("walking up the ramp reaches the visitor entrance and it opens from outside")))return;
  Go(3,1.2);return;
 }
 if(Stage==3){M->Player->TestMove=FVector2D(0,-1);Go(4);return;}
 if(Stage==4)
 {
  if(M->Player->Pos.X>-3)return;
  M->Player->TestMove.Reset();
  if(!Check(FMath::Abs(M->Player->FloorY)<.05,TEXT("actual walking crosses the visitor entrance into the corridor")))return;
  M->Player->Place(35.4,11,0);M->Player->Pitch=.05;Go(5,1.5);return;
 }
 if(Stage==5){Shot(TEXT("garage-start-door-outside.png"));Go(6,.6);return;}
 if(Stage==6){M->Player->Place(32.6,-4.4,-2.6);M->Player->Pitch=-.1;Go(7,1.5);return;}
 if(Stage==7){Shot(TEXT("garage-start-jeep-inside.png"));Go(8,.6);return;}
 if(Stage==8){W->Roam(TEXT("saro"));Go(9);return;}
 if(Stage==9)
 {
  if(W->bBusy||!W->bDriving||!D)return;
  W->TestInput=[]{FS47DriveIn I;I.Throttle=.6;return I;};Go(10,5.);return;
 }
 if(Stage==10)
 {
  if(!D)return;
  const double Z=D->Pos.Z;
  if(!Check(Z>0&&Z<5.3,FString::Printf(TEXT("Jeep faces the door and the closed door stops it, at z %.2f"),Z)))return;
  W->TestInput=[W]{FS47DriveIn I;const auto* C=W->CabDrive();I.Throttle=C&&C->Speed>.08?-1.:0.;return I;};Go(11,2.);return;
 }
 if(Stage==11)
 {
  W->TestInput=nullptr;if(!Check(W->GetOutHere(),TEXT("driver can get out in the garage")))return;Go(12);return;
 }
 if(Stage==12)
 {
  if(W->bBusy||W->bDriving)return;
  if(!Check(bool(M->Player->OnUse),TEXT("door control callback ready")))return;
  M->Player->OnUse(TEXT("eastwing:garage-door-in"));Go(13);return;
 }
 if(Stage==13)
 {
  if(Garage()<.95)return;
  Check(true,TEXT("the wall control raises the garage door"));W->Roam(TEXT("saro"));Go(14);return;
 }
 if(Stage==14)
 {
  if(W->bBusy||!W->bDriving||!D)return;
  W->TestInput=[]{FS47DriveIn I;I.Throttle=.6;return I;};Go(15);return;
 }
 if(Stage==15)
 {
  if(!D||D->Pos.Z<11)return;
  W->TestInput=nullptr;Check(true,FString::Printf(TEXT("Jeep drives out through the open door, down the ramp, at %.2f %.2f"),D->Pos.X,D->Pos.Z));
  Finish(true,TEXT("New game outside the visitor entrance, all doors shut, Jeep in the closed garage and able to leave once the door is up."));return;
 }
}
