#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Album.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47Inter.h"
#include "S47LeftGear.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Openings.h"
#include "S47FieldLife.h"
#include "S47Places.h"
#include "S47MotelNight.h"
#include "S47NightRhythm.h"
#include "S47FreeformWorld.h"
#include "S47Legs.h"
#include "S47Three.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

// -S47Freeform -S47FreeformTest -S47FreeLifeTest: the chain as a player meets it, with the live rules on. A field visit from its
// own draw and dwell, taken, the gear left behind, the case, the note, going back for the gear, and the originals untouched.
void FS47FreeformTest::StepFreeLife(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>900){Finish(false,TEXT("life journey exceeded its time"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& F=*M->Freeform;if(!F.FarmSite||!F.Taken||!F.Gear)return;auto& Site=*F.FarmSite;
 static int32 ShotsBefore=0,Night=0;static double SeenClock=0;
 if(Stage==10||Stage==20)
 {
  // Waiting in the live night, nothing here moves the clock but the night itself (seconds of it a frame). A jump of half an hour,
  // or back, is a hand on the test's window (F10's events, a rest, a load), not the draw: say so (life run of 9 November, 15:57:
  // keys and an F10 line in the window took the clock from 02:31 to 21:00 and the run blamed the night).
  const double Now=F.Clock();
  if(Now>SeenClock+1800||Now<SeenClock-1)
  {
   const auto Hm=[](double C){const int32 Mi=int32(FMath::Fmod(FMath::Max(0.,C),86400.)/60.);return FString::Printf(TEXT("%02d:%02d"),Mi/60,Mi%60);};
   return Finish(false,FString::Printf(TEXT("the clock was moved from outside the test, %s to %s (day %d) at stage %d: input in the test window (F10, a rest, a load)? run again untouched"),*Hm(SeenClock),*Hm(Now),F.Day(),Stage));
  }
  SeenClock=Now;
 }
 if(Stage==0)
 {
  if(Age<3)return;
  M->UI->Close(true);F.TestLiveRules();F.TestEndFirstShift();
  if(F.Director)F.Director->TestHush(true);
  if(M->FieldRadio)M->FieldRadio->TestOwn();
  if(M->FieldCamera)M->FieldCamera->bHave=true;
  ShotsBefore=M->Night&&M->Night->Album?M->Night->Album->Shots().Num():0;
  // The first night after five that is not a calm one, an hour into their own hours.
  Night=6;while(F.QuietNight(Night))++Night;
  const double Midnight=double(Night)*86400.;
  const double Target=Midnight+(F.BodyStartHour(Night)+.25)*3600.;
  if(!Check(F.TestAdvance(Target-F.Clock())&&F.TheirHoursNow(F.Clock()),TEXT("the clock stands in the night's own hours")))return;
  if(!Check(!F.TheirHoursNow(Midnight+.5*3600.),TEXT("half an hour after midnight is not their hours")))return;
  // Past the farm, away from the station, in the open.
  M->Player->Place(90,620,0);
  Site.TestFieldRoll(0.f);SeenClock=F.Clock();Stage=10;At=Age;return;
 }
 if(Stage==10)
 {
  if(!F.TheirHoursNow(F.Clock()))return Finish(false,TEXT("the night ran out before the draw"));
  if(!Site.FieldVisit())
  {
   // The draw waits for the rhythm as well as the dwell. TestHush stills the director, not the night's happenings and unease
   // (S47Happenings): one that begins just before the dwell is done holds the draw for its spacing (2 to 6 minutes of the clock,
   // up to half a minute here) and, ending, for the rhythm's rest after it (45 s): room for both, not a fail for a night's own life.
   if(Age-At>FS47FarmSite::FieldDwellSeconds+100)return Finish(false,FString::Printf(TEXT("no field visit from the draw: drawn night %d, want %d, indoors %d"),Site.FieldDrawnNight(),Night,F.Taken->Indoors()));
   return;
  }
  if(!Check(Age-At>=FS47FarmSite::FieldDwellSeconds-1&&Site.FieldDrawnNight()==F.Day(),TEXT("a field visit starts from its own draw, once they have been out there a while")))return;
  if(!Check(Site.VisitStage()>=1&&Site.VisitWantsGround(),TEXT("the visit has begun")))return;
  Stage=20;At=Age;return;
 }
 if(Stage==20)
 {
  if(Site.VisitStage()<2)
  {
   if(Age-At>60)return Finish(false,TEXT("nothing came out on the field visit"));
   return;
  }
  UE_LOG(LogTemp,Display,TEXT("S47: free life: field visit body kind %d, %.1f m off, %.0f s"),Site.VisitKind(),Site.VisitBodyDistance(),Age-At);
  if(!Check(Site.VisitBodyDistance()>=0,TEXT("a body is out on the field visit")))return;
  if(!Check(F.Taken->TakeNow(ES47Taker::Tall,TEXT("the open ground past the SARO fence")),TEXT("reached by it")))return;
  Stage=30;At=Age;return;
 }
 if(Stage==30)
 {
  if(F.Taken->Busy()){if(Age-At>40)return Finish(false,TEXT("taken never finished"));return;}
  if(!Check(F.Gear->IsLeft(TEXT("radio"))&&F.Gear->IsLeft(TEXT("camera")),TEXT("the tall one leaves the set and the camera where it happened")))return;
  if(!Check(!M->FieldRadio->Owned()&&!M->FieldCamera->bHave&&F.CameraLeftOut()&&M->FieldRadio->LeftOut(),TEXT("neither is on the player, and neither has gone back to the desk or bench")))return;
  const int32 Shots=M->Night&&M->Night->Album?M->Night->Album->Shots().Num():0;
  if(!Check(Shots>=ShotsBefore,TEXT("no photograph has gone")))return;
  const auto* Lost=F.CaseFile().Get(FS47FreeformCases::LostLine);
  if(!Check(Lost&&Lost->bOpen,TEXT("the case of the lost hours is open")))return;
  if(!Check(F.Taken->LastNote().Contains(TEXT("must still be where I stood")),TEXT("the note says what is missing")))return;
  const auto Saved=F.Save();
  if(!Check(Saved->HasField(TEXT("gear")),TEXT("what lies out there is saved")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: free life: %s"),*F.Taken->LastNote());
  Stage=40;At=Age;return;
 }
 static int32 PlaceIndex=0,RoundIndex=0,RoundPay=0,RoundSheet=0;
 if(Stage==50)
 {
  // Each place by daylight, on its own ground, seen from a few metres, with a picture for a person to judge.
  const double Noon=(FMath::FloorToDouble(F.Clock()/86400.)+1)*86400.+11*3600;
  if(!Check(F.TestAdvance(Noon-F.Clock()),TEXT("daylight for the places")))return;
  F.TestGiveKit(TEXT("magnetometer"));F.TestGiveKit(TEXT("cot"));F.TestGiveKit(TEXT("alarm"));
  M->World->Enter(TEXT("road"));PlaceIndex=0;Stage=51;At=Age;return;
 }
 if(Stage==51)
 {
  const auto& Defs=FS47Places::Defs();
  while(PlaceIndex<Defs.Num()&&FS47Places::AtSaro(Defs[PlaceIndex].Id))++PlaceIndex; // (T16-17: the motel's desk and the old house's two are on SARO's ground)
  if(PlaceIndex>=Defs.Num()){Stage=60;At=Age;return;}
  double X=0,Z=0;if(!Check(F.Places->Spot(Defs[PlaceIndex].Id,X,Z),TEXT("the place has a spot")))return;
  const FVector Road=S47Legs::ORIGINS(TEXT("road"));
  M->Player->Place(Road.X+X,Road.Z+Z+5,0);M->Player->Pitch=-.05;
  Stage=52;At=Age;return;
 }
 if(Stage==52)
 {
  if(Age-At<3)return;
  const auto& D=FS47Places::Defs()[PlaceIndex];
  double X=0,Z=0;F.Places->Spot(D.Id,X,Z);const FVector Road=S47Legs::ORIGINS(TEXT("road"));
  // A place may be under foot of the land's own rise: aim at it, not at a compass point.
  const auto* Thing=M->Inter->Get(FS47Places::UseId(D.Id));
  const FVector To=Thing&&Thing->Object?Thing->Object.WorldBox().GetCenter():FVector(Road.X+X,M->Player->FloorY+1.2,Road.Z+Z);const FVector Eye=S47Three::ToThree(M->Player->EyeUE());const FVector Way=To-Eye;
  M->Player->Yaw=FMath::Atan2(-Way.X,-Way.Z);M->Player->Pitch=FMath::Atan2(Way.Y,FVector2D(Way.X,Way.Z).Size());
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("FreeformTest")/FString::Printf(TEXT("place-%s.png"),D.Id),false,false);
  Stage=53;At=Age;return;
 }
 if(Stage==53)
 {
  if(Age-At<.6)return;
  const auto& D=FS47Places::Defs()[PlaceIndex];
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  UE_LOG(LogTemp,Display,TEXT("S47: free life: at %s the eye picks '%s', floor %.2f"),D.Id,*Pick,M->Player->FloorY);
  if(!Check(Pick==FS47Places::UseId(D.Id),FString::Printf(TEXT("the eye can pick %s from a few steps off, got '%s'"),D.Id,*Pick)))return;
  if(!Check(F.Places->Use(D.Id)&&F.HasFinding(FString(TEXT("place_"))+D.Id)&&F.HasFinding(FString(TEXT("place_"))+D.Id+TEXT("_needle")),TEXT("each place has its one thing to find, and the needle reads it")))return;
  M->UI->Close(true);++PlaceIndex;Stage=51;At=Age;return;
 }
 if(Stage==60)
 {
  FS47MapView V;F.Places->FillMap(V);
  if(!Check(V.Pins.Num()>=9,TEXT("all of them are on the map now")))return;
  if(!Check(F.TownTrust()>=1,TEXT("Caliche has noticed")))return;
  // The alarm: trips, gets a few seconds out, is put out, wants a fuse.
  M->World->Enter(TEXT("saro"));M->Player->Place(9.2,1.2,0);F.AddItem(TEXT("fuse"),1);
  F.YardAlarm->TestTrip(0.f);Stage=61;At=Age;return;
 }
 if(Stage==61)
 {
  if(Age-At<1)return;
  if(!Check(F.YardAlarm->Ringing()&&F.YardAlarm->Dead()&&F.HasFinding(FString::Printf(TEXT("alarm-day%d"),F.Day())),TEXT("the alarm goes off and is on record, and this one is being put out")))return;
  Stage=62;At=Age;return;
 }
 if(Stage==62)
 {
  if(Age-At<6)return;
  if(!Check(!F.YardAlarm->Ringing(),TEXT("it has stopped")))return;
  const int32 Fuses=F.InventoryCount(TEXT("fuse"));M->Inter->UseId(TEXT("ff:alarm"));
  if(!Check(!F.YardAlarm->Dead()&&F.InventoryCount(TEXT("fuse"))==Fuses-1,TEXT("a fuse at its box mends it")))return;
  Stage=70;At=Age;return;
 }
 if(Stage==70)
 {
  // A motel door keeps out the night: no wake, and the price paid.
  int32 N=F.Day();while(F.QuietNight(N)||N<2)++N;
  const double Evening=double(N-1)*86400.+22*3600.;
  if(!Check(F.TestAdvance(FMath::Max(0.,Evening-F.Clock())),TEXT("the evening")))return;
  F.Earn(50);const int32 Cash=F.Budget();
  const double Morning=(FMath::FloorToDouble((F.Clock()-8*3600)/86400.)+1)*86400.+8*3600;
  if(!Check(F.RestAt(Morning,FS47FreeformSession::ERest::Motel)&&F.Clock()==Morning&&F.Budget()==Cash-FS47FreeformSession::MotelRoom,TEXT("a night at the motel: to morning, unbroken, for the price of a room")))return;
  {
   // MOTEL-PAY-20261109: the desk's own way in. Woken in the night (the door handle here; the big night wakes a room the same way)
   // and back at the desk, the room is the one already paid for: one night, one price.
   if(!Check(F.MotelNight.IsValid(),TEXT("the motel's nights")))return;
   const double Evening2=double(F.Day()-1)*86400.+22*3600.;
   if(!Check(F.TestAdvance(FMath::Max(0.,Evening2-F.Clock())),TEXT("the next evening")))return;
   F.Earn(50);const int32 Before=F.Budget(),Price=F.MotelPrice();
   const double Morning2=(FMath::FloorToDouble((F.Clock()-8*3600)/86400.)+1)*86400.+8*3600;
   F.MotelNight->DevKind(int32(FS47MotelNight::EKind::Handle));
   if(!Check(F.MotelNight->TakeRoom(Morning2)&&F.Clock()<Morning2&&F.Budget()==Before-Price&&F.MotelNight->RoomPaid(F.Clock()),TEXT("a room taken at the desk, paid, and the night wakes the player before morning")))return;
   if(M->UI)M->UI->Close(true);
   for(int32 I=0;I<3&&F.Clock()<Morning2;++I){if(!Check(F.MotelNight->TakeRoom(Morning2),TEXT("back to the room from the desk")))return;if(M->UI)M->UI->Close(true);}
   if(M->UI)M->UI->Close(true);
   if(!Check(F.Clock()==Morning2&&F.Budget()==Before-Price&&!F.MotelNight->RoomPaid(F.Clock()),TEXT("back at the desk the same night: the same room, not paid for twice")))return;
  }
  // A cot out on the land: on a night that is not calm, something comes by.
  int32 C=F.Day();while(F.QuietNight(C)||C<2)++C;
  const double Late=double(C-1)*86400.+22*3600.;
  F.TestAdvance(FMath::Max(0.,Late-F.Clock()));
  const double Next=(FMath::FloorToDouble((F.Clock()-8*3600)/86400.)+1)*86400.+8*3600;const int32 Cots=F.Budget();
  if(!Check(F.RestAt(Next,FS47FreeformSession::ERest::Field)&&F.Clock()<Next&&F.LastWakeClock()==F.Clock()&&F.TheirHoursNow(F.Clock())&&F.Budget()==Cots,TEXT("a cot out on the land: woken in their hours, at no charge")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: free life: stores, places, alarm, motel and cot PASS"));
  // The truck: each kind of trouble has its own way out, and the county is there for all of them.
  auto& Care=*F.Care;const int32 Spare=F.InventoryCount(TEXT("spare")),Water=F.InventoryCount(TEXT("water"));const double Before=F.Clock();
  if(!Check(Spare==1&&Water==2,TEXT("the truck starts with a spare and two cans of water")))return;
  Care.TestStick();Care.TestFlat();Care.TestWater(0);
  if(!Check(Care.NeedsRescue()&&Care.DigOut()&&Care.ChangeWheel()&&Care.TopUp()&&!Care.NeedsRescue()&&F.Clock()>Before&&F.InventoryCount(TEXT("spare"))==Spare-1&&F.InventoryCount(TEXT("water"))==Water-1,TEXT("spade, jack and can each get the player going again, in time and stock")))return;
  Care.TestStick();Care.TestFlat();
  if(!Check(!Care.ChangeWheel()&&Care.Flat(),TEXT("no spare left: the wheel stays flat, and says so")))return;
  F.Earn(100);Care.TestLeg(10000);const int32 CashBefore=F.Budget(),FeeDue=Care.TowFee();const double Rescue=F.Clock();F.Field->TestRoadside();
  if(!Check(!Care.NeedsRescue()&&F.Clock()>=Rescue+75*60-1,TEXT("roadside service gets the player out of all of it, for time (a long way out: 75 minutes)")))return;
  if(!Check(FeeDue==60&&F.Budget()==CashBefore-FeeDue,TEXT("and the tow is paid in cash: $60 at ten kilometres")))return;
  Care.TestStick();Care.TestLeg(2000);const int32 OwedBefore=F.Arrears(),FeeShort=Care.TowFee();
  F.Spend(F.Budget());const int32 Broke=F.Budget();F.Field->TestRoadside();
  if(!Check(FeeShort==40&&Broke==0&&F.Budget()==0&&F.Arrears()==FMath::Min(int32(FS47FreeformSession::ArrearsCap),OwedBefore+FeeShort)&&!Care.NeedsRescue(),TEXT("with no cash the tow goes on the weekly sheet and is never refused")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: free life: truck care PASS"));
  // The trip camera, left at a place, read in the morning.
  F.Field->TestBuiltTripCamera();
  if(!Check(F.TripCamera->CanSet()&&F.TripCamera->SetHere(TEXT("the old mine on the west side"),false)&&F.TripCamera->Out()&&!F.TripCamera->CanSet(),TEXT("set once, out until read")))return;
  Stage=80;At=Age;return;
 }
 if(Stage==80)
 {
  const double Morning=(FMath::FloorToDouble((F.Clock()-8*3600)/86400.)+1)*86400.+9*3600;
  if(!Check(F.TestAdvance(Morning-F.Clock()),TEXT("the next morning")))return;
  const int32 Records=F.FindingCount();
  if(!Check(F.TripCamera->Collect()&&!F.TripCamera->Out()&&F.FindingCount()==Records+1,TEXT("read in the morning: one record, for the night it stood out")))return;
  M->UI->Close(true);
  // The round: its three stops, by daylight, on foot.
  F.Earn(0);const int32 Pay=F.Budget();
  const auto Stops=FS47DayRound::StopsFor(F.Seed(),F.Day());
  for(const FString& S:Stops)if(!Check(F.Field->InteractionPoint(S).X!=0||F.Field->InteractionPoint(S).Z!=0,TEXT("each stop of the round has a place in the world")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: free life: trip camera PASS, round stops %s"),*FString::Join(Stops,TEXT(",")));
  RoundIndex=0;RoundPay=Pay;RoundSheet=F.Arrears();Stage=81;At=Age;return;
 }
 if(Stage==81)
 {
  const auto Stops=FS47DayRound::StopsFor(F.Seed(),F.Day());
  if(RoundIndex>=Stops.Num())
  {
   if(!Check(F.Round->DoneToday()&&F.Budget()+(RoundSheet-F.Arrears())>=RoundPay+FS47DayRound::RoundPay&&F.HasFinding(FString::Printf(TEXT("round-d%d"),F.Day())),TEXT("three stops make a round, paid and on record")))return;
   UE_LOG(LogTemp,Display,TEXT("S47: free life: round PASS"));
   Finish(true,TEXT("field visit, taken, gear, case, places, stores, alarm, rests, truck, trip camera and round as played"));return;
  }
  if(Age-At<1.2)return;At=Age;
  const FString Stop=Stops[RoundIndex++];
  // The feeder stands out on the road; the rest are at the station.
  M->World->Enter(Stop==TEXT("ff:feeder")?TEXT("road"):TEXT("saro"));
  const FVector P=F.Field->InteractionPoint(Stop);M->Player->Place(P.X+1,P.Z,0);
  return;
 }
 if(Stage==40)
 {
  // Back to it on foot, or as good as: it is the player's hands that pick it up.
  const auto& Items=F.Gear->Items();
  if(Items.IsEmpty())
  {
   if(!Check(M->FieldRadio->Owned()&&M->FieldCamera->bHave&&!F.CameraLeftOut(),TEXT("both are back in hand")))return;
   if(!Check(F.HasFinding(TEXT("lost_ground")),TEXT("going back gave the case its trace")))return;
   UE_LOG(LogTemp,Display,TEXT("S47: free life: field visit, taken, gear, case and fetch PASS"));
   Stage=50;At=Age;return;
  }
  const FVector Where=Items[0].At;
  if(Age-At<1)return;At=Age;
  M->Player->Place(Where.X+.6,Where.Z,0);
  M->Inter->UseId(TEXT("ff:left:")+Items[0].Item);
  return;
 }
}
