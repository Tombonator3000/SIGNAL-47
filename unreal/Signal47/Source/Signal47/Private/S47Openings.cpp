#include "S47Openings.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47FarmSite.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Glimpses.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

const TArray<FS47KitOffer>& FS47Openings::Defs()
{
 static const TArray<FS47KitOffer> Table={
  {TEXT("spotlight"),TEXT("Roof spotlight for the truck"),35,TEXT("one branch of the array back in service, so the stores will talk to you"),TEXT("F in the cab. It throws light a long way, and it can be seen from a long way off.")},
  {TEXT("cot"),TEXT("Field cot in the truck"),20,TEXT("any field record kept"),TEXT("Sleep out at any place on your map. There is no door out there, and you will not sleep through much.")},
  {TEXT("magnetometer"),TEXT("Survey magnetometer, used"),40,TEXT("a place found out on the land, or marks at a site kept as a record"),TEXT("On foot, the needle pulls toward marks, gear and places you haven't checked yet, and it gives a reading on what you find. It can't tell you what they are.")},
  {TEXT("logger2"),TEXT("Second field logger and contact pickups"),45,TEXT("THE REPEATED INTERVAL concluded, with the first logger on loan"),TEXT("Makes the two-site test on VISITORS AROUND THE BASE possible: the roof and a field post at the same minute.")},
  {TEXT("film"),TEXT("Fast film and a better lens"),30,TEXT("a photograph with a figure in it"),TEXT("One extra roll, fast enough for poor light, and $5 more for each registered incident.")},
  // GEIGER-20261108: not a counter yet, a part of one. The player builds it at the bench (FS47Workshop "geiger", S47Happenings.h).
  {TEXT("geiger_tube"),TEXT("Geiger-Muller tube and a meter movement"),25,TEXT("any field record kept: a place, marks, or scorched ground"),TEXT("Parts, not a counter. Build one at the CALIBRATION / REPAIR bench: a recorder housing, a lead and a dry cell. On foot it ticks; over burned or pressed ground it ticks faster.")},
  {TEXT("alarm"),TEXT("Yard lights and alarm horn"),60,TEXT("the county on speaking terms with the station"),TEXT("Goes off when anything comes within fifty yards of the station. It warns you and stops nothing. Sometimes it goes dead in the middle of an alarm; a fuse at its box fixes it.")},
  // M7 (C): two dear ones late in the month (S47OpeningsLocal::Late), so that a careful budget has something worth saving for in weeks 3
  // and 4. FS47FreeformSession::KitWaits opens them from day 15 (the county's surplus sale).
  {TEXT("transfer"),TEXT("Automatic transfer switch for the backup generator"),90,TEXT("the county's surplus sale, from day 15"),TEXT("When the feeder fails, it throws the backup generator in by itself, from its own tank or a can in stock. You hear it clunk over from anywhere in the building."),3},
  {TEXT("array_alarm"),TEXT("Trip wires along the antenna field"),110,TEXT("the yard alarm fitted, and the county's surplus sale from day 15"),TEXT("Wires the yard alarm out along the dishes: the horn goes for anything among the antennas, not only at the yard. It still stops nothing, and out there it is easier to knock out."),3},
  // M7 (Tom, 9 November): the truck you have, made better, not a new one (FS47Outfit). Felt in the driving and in what it carries.
 // CHAIN-2-20261109: from week 2 each waits for something brought in from the field as well (FS47FreeformSession::KitWaits).
  {TEXT("truck_tires"),TEXT("Mud-and-sand tires for the truck"),45,TEXT("a week on the job (day 8), when the county signs for tires, and a field record kept out on the land"),TEXT("More grip and more speed on gravel and dirt, and the wheels dig in half as fast in sand. The blacktop is no different."),2},
  {TEXT("truck_rack"),TEXT("Bed rack and tie-downs for the truck"),40,TEXT("a week on the job (day 8) and something brought back from out on the land"),TEXT("A parcel from the counters can be more than twice as big: twenty items in the rack instead of eight."),2},
  {TEXT("truck_tank"),TEXT("Auxiliary tank behind the cab"),55,TEXT("a week on the job (day 8) and a field record kept out on the land"),TEXT("Six units of fuel instead of four: about 72 km between pumps instead of 48. A full fill costs more."),2},
  {TEXT("truck_engine"),TEXT("Engine rebuild with a four-barrel carburetor"),75,TEXT("a week on the job (day 8) and two kinds of record kept out on the land at night"),TEXT("The truck pulls harder from a stand and runs faster on every road, and drinks about a tenth more fuel."),2},
  // and field gear that opens the night: what a better ear and a better eye can bring back
  {TEXT("field_recorder"),TEXT("Better field recorder and a gun microphone"),60,TEXT("a week on the job (day 8) and a cassette recorded out in the field"),TEXT("The tape hears twice as far, 140 m instead of 70, and catches what is out there from farther off. The cassettes cost the same."),2},
  {TEXT("night_camera"),TEXT("Image intensifier for the field camera"),85,TEXT("a week on the job (day 8) and the fast film in hand"),TEXT("Surplus night glass on the lens: the film sees far more in the dark than your eye does. It won't invent anything; it only shows what was really there."),2},
  {TEXT("rdf"),TEXT("Radio direction finder loop for the truck"),120,TEXT("the county's surplus sale, from day 15, and three kinds of night record from out on the land, or a field time that matched the array to the minute"),TEXT("A loop antenna on the cab roof. At night, sitting in the cab, you can turn it toward anything transmitting within about two miles and hear which direction it's coming from. It can't tell you what is sending."),3},
 };
 return Table;
}
const FS47KitOffer* FS47Openings::Def(const FString& Id){return Defs().FindByPredicate([&](const FS47KitOffer& O){return Id==O.Id;});}
int32 FS47Openings::FirstDay(const FString& Id){const auto* O=Def(Id);return O?(FMath::Max(1,O->Week)-1)*7+1:1;}
const TCHAR* FS47Openings::Kind(const FString& Id)
{
 // D45-20261110: the truck and what rides on it, the gear carried on foot, and what is fitted at the station
 if(Id==TEXT("spotlight")||Id==TEXT("cot")||Id==TEXT("rdf")||Id.StartsWith(TEXT("truck_")))return TEXT("TRUCK");
 if(Id==TEXT("alarm")||Id==TEXT("transfer")||Id==TEXT("array_alarm"))return TEXT("STATION");
 return Def(Id)?TEXT("FIELD"):TEXT("");
}
namespace S47OpeningsLocal
{
 // The late, dear ones (from day 15): what a full budget in weeks 3 and 4 can go on.
 bool Late(const FString& Id){const auto* O=FS47Openings::Def(Id);return O&&O->Week>=3;}
 // The trip wires along the antenna field: this close to a dish counts as among them.
 constexpr double DishReach=14;
 constexpr float ArrayKnockedOutChance=.45f;
}
bool FS47Openings::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s openings: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 TSet<FString> Ids;bool bWords=true;int32 Total=0,LateTotal=0,LateCount=0,Cheapest=1000000,MidTotal=0,MidCount=0,MidDearest=0,MidCheapest=1000000,EarlyDearest=0;
 for(const auto& O:Defs())
 {
  Ids.Add(O.Id);bWords&=O.Price>0&&FCString::Strlen(O.Waits)>3&&FCString::Strlen(O.What)>10;
  if(S47OpeningsLocal::Late(O.Id)){LateTotal+=O.Price;++LateCount;Cheapest=FMath::Min(Cheapest,O.Price);}
  else if(O.Week==2){MidTotal+=O.Price;++MidCount;MidDearest=FMath::Max(MidDearest,O.Price);MidCheapest=FMath::Min(MidCheapest,O.Price);}
  else{Total+=O.Price;EarlyDearest=FMath::Max(EarlyDearest,O.Price);}
 }
 Check(Ids.Num()==Defs().Num()&&bWords,TEXT("each has a price, what it waits for, and what it does"));
 // D45-20261110: each stands on OPS SUPPLIES under one of three kinds
 {
  int32 Trucks=0,Fields=0,Stations=0;
  for(const auto& O:Defs()){const FString K=Kind(O.Id);Trucks+=K==TEXT("TRUCK")?1:0;Fields+=K==TEXT("FIELD")?1:0;Stations+=K==TEXT("STATION")?1:0;}
  Check(Trucks+Fields+Stations==Defs().Num()&&Trucks==7&&Fields==6&&Stations==3&&FString(Kind(TEXT("nothing"))).IsEmpty(),TEXT("D45: each offer is tagged TRUCK, FIELD or STATION"));
 }
 Check(Total<=300,TEXT("all the early ones together are about five days' full pay"));
 // M7 (C): the money model for days 1 to 30 leaves $150 to $300 a week unspent in weeks 3 and 4 on two or three cards an evening.
 // M7 (Tom, 9 November, E's model): prices step up by the week; two cards an evening cannot buy all of it in a month.
 Check(LateCount==3&&Cheapest>=90&&LateTotal>=300&&LateTotal<=360,TEXT("M7: three dear late ones from day 15, $90 and up"));
 Check(MidCount==6&&MidCheapest>=40&&MidDearest<=Cheapest&&MidDearest>EarlyDearest&&MidTotal>=300&&MidTotal<=400,TEXT("M7: six from week 2 (the truck and the better field gear), dearer together than the early ones and each cheaper than the late"));
 Check(FirstDay(TEXT("truck_engine"))==8&&FirstDay(TEXT("rdf"))==15&&FirstDay(TEXT("cot"))==1,TEXT("M7: week 2 opens on day 8, week 3 on day 15"));
 bool bTruck=true;for(const TCHAR* Truck:{TEXT("truck_tires"),TEXT("truck_rack"),TEXT("truck_tank"),TEXT("truck_engine")})bTruck&=Def(Truck)!=nullptr;
 Check(bTruck,TEXT("M7: the truck's own upgrades are on the stores list"));
 return OK;
}

// ---------- the truck's spotlight
void FS47TruckLamp::Stop()
{
 bOn=false;
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())if(auto* L=Lamp.Get())L->SetVisibility(false);
}
bool FS47TruckLamp::Toggle()
{
 if(!M||!M->Freeform||!M->Freeform->HasKit(TEXT("spotlight"))||!M->World||!M->World->bDriving)return false;
 bOn=!bOn;
 if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.5f});
 UE_LOG(LogTemp,Display,TEXT("S47: truck spotlight %s"),bOn?TEXT("on"):TEXT("off"));
 return true;
}
void FS47TruckLamp::Step(double Dt)
{
 static_cast<void>(Dt);
 if(!M||!M->Freeform||!M->Web||!M->World)return;
 auto* D=M->World->CabDrive();
 const bool bShow=bOn&&M->World->bDriving&&D&&D->bEngine;
 auto* L=Lamp.Get();
 if(!L&&bShow)
 {
  L=NewObject<USpotLightComponent>(M->Web.Get(),TEXT("S47TruckSpot"));L->SetupAttachment(M->Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);L->RegisterComponent();
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetLightColor(FLinearColor(1,.93f,.78f));L->SetInnerConeAngle(5);L->SetOuterConeAngle(11);L->SetAttenuationRadius(22000);L->SetCastShadows(true);L->SetVolumetricScatteringIntensity(.6f);
  Lamp=L;
 }
 if(!L)return;
 L->SetVisibility(bShow);if(!bShow){L->SetIntensity(0);return;}
 const FVector Forward=FVector(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
 L->SetWorldLocation(S47Three::ToUE(D->Pos+FVector(0,1.9,0)+Forward*.4));
 L->SetWorldRotation(S47Three::DirToUE(Forward+FVector(0,-.03,0)).Rotation());
 L->SetIntensity(90000.f);
 // Only in the dark does it count against them.
 const double Hour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;
 if((Hour>=19||Hour<6)&&LitNight!=M->Freeform->Day()){LitNight=M->Freeform->Day();UE_LOG(LogTemp,Display,TEXT("S47: truck spotlight lit on night %d: the night's draws lean their way"),LitNight);}
}

// ---------- the yard alarm
void FS47YardAlarm::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);bDead=Saved.IsValid()&&S47J::Bool(Saved,TEXT("dead"));bNear=false;RingLeft=HornIn=0;
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){bBuilt=false;Lamps.Reset();Box=FS47Obj();}
}
void FS47YardAlarm::Stop()
{
 Life.Reset();RingLeft=0;
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())for(auto& L:Lamps)if(auto* C=L.Get())C->SetVisibility(false);
}
S47J::FObj FS47YardAlarm::Save() const
{
 auto O=S47J::New();if(bDead)S47J::Set(O,TEXT("dead"),true);return O;
}
bool FS47YardAlarm::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||M->Web->Area()!=TEXT("saro")||!IsValid(M->Inter)||!M->Freeform||!M->Freeform->Field)return false;
 const int32 Parent=M->Web->Handle(TEXT("ext.group"));if(Parent<0)return false;
 // A grey box on the wall by the service door, and three lamps over the yard.
 Box=M->Freeform->Field->StageBox(TEXT("S47YardAlarmBox"),FVector(11.2,1.5,-.2),FVector(.4,.55,.16),0x6f736e,Parent);if(!Box)return false;
 for(const FVector& At:{FVector(9,5,-6),FVector(14,5,8),FVector(-4,5,10)})
 {
  auto* L=NewObject<UPointLightComponent>(M->Web.Get());L->SetupAttachment(M->Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);L->RegisterComponent();
  L->SetLightColor(FLinearColor(1,.82f,.55f));L->SetAttenuationRadius(3500);L->SetCastShadows(false);L->SetIntensity(0);L->SetVisibility(false);L->SetWorldLocation(S47Three::ToUE(At));Lamps.Add(L);
 }
 Scene=M->Web;
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable I;I.Id=TEXT("ff:alarm");I.Object=Box;I.Range=2.4;
 I.Label=[this,Guard]{if(!Guard.IsValid()||!M->Freeform||!M->Freeform->HasKit(TEXT("alarm")))return FString();return bDead?FString(TEXT("Yard alarm / dead. Fit a fuse")):FString(TEXT("Yard alarm / armed"));};
 I.Use=[this,Guard]{if(Guard.IsValid())Mend();};I.Examine=I.Label;
 M->Inter->Add(I);M->Web->MarkUsable(Box.I,I.Id);
 bBuilt=true;return true;
}
void FS47YardAlarm::Mend()
{
 if(!bDead||!M||!M->Freeform)return;
 if(!M->Freeform->Consume(TEXT("fuse"))){if(M->UI)M->UI->Toast(TEXT("It needs a fuse, and you have none on you."),4);return;}
 bDead=false;if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.5f});
 if(M->UI)M->UI->Toast(TEXT("New fuse. The yard alarm's lamp comes back on."),4);
 UE_LOG(LogTemp,Display,TEXT("S47: yard alarm mended"));M->RequestAutosave();
}
namespace S47OpeningsLocal
{
 // M7 (C): the trip wires along the antenna field ("array_alarm"): a figure this close to any dish.
 bool AmongDishes(const US47Main* M)
 {
  if(!M||!M->Web||M->Web->Area()!=TEXT("saro")||!M->Night||!M->Night->Glimpses)return false;
  for(const auto& I:M->Night->Glimpses->Individuals())
   for(int32 D=0;D<M->Web->DishCount();++D)if(FVector2D::Distance(FVector2D(I.Pos.X,I.Pos.Z),FVector2D(M->Web->DishX(D),M->Web->DishZ(D)))<DishReach)return true;
  return false;
 }
}
bool FS47YardAlarm::BodyNear() const
{
 if(!M||!M->Web||M->Web->Area()!=TEXT("saro")||!M->Freeform)return false;
 const FVector2D Station(9,0);
 if(M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())if(FVector2D::Distance(FVector2D(I.Pos.X,I.Pos.Z),Station)<Reach)return true;
 if(M->Freeform->FarmSite)if(const auto Eye=M->Freeform->FarmSite->VisitBodyEye())if(FVector2D::Distance(FVector2D(Eye->X,Eye->Z),Station)<Reach)return true;
 return false;
}
void FS47YardAlarm::Show()
{
 const bool bRing=RingLeft>0&&!bDead;
 for(auto& L:Lamps)if(auto* C=L.Get()){C->SetVisibility(bRing);C->SetIntensity(bRing?6000.f:0.f);}
}
void FS47YardAlarm::Step(double Dt)
{
 if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 if(!F.HasKit(TEXT("alarm")))return;
 if(!Build())return;
 const bool bYard=BodyNear();
 const bool bField=!bYard&&F.HasKit(TEXT("array_alarm"))&&S47OpeningsLocal::AmongDishes(M);
 const bool bHere=bYard||bField;
 const bool bTrip=(bHere&&!bNear)||bTestTrip;bNear=bHere;bTestTrip=false;
 if(bTrip&&!bDead&&F.HasPowerForReactiveLight())
 {
  RingLeft=RingSeconds;HornIn=0;
  const int32 S=int32(FMath::Fmod(F.Clock(),86400.));
  F.RecordEvidence(FString::Printf(TEXT("alarm-day%d"),F.Day()),bField?FString::Printf(TEXT("The trip wires along the antenna field set the yard alarm off at %02d:%02d. Something was out among the dishes. It does not say what."),S/3600,(S/60)%60)
   :FString::Printf(TEXT("The yard alarm went off at %02d:%02d. Something was within its reach. It does not say what."),S/3600,(S/60)%60));
  if(M->UI)M->UI->Toast(bField?TEXT("The yard alarm is going off. The trip wires, out by the dishes."):TEXT("The yard alarm is going off."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: yard alarm trips at %02d:%02d (%s)"),S/3600,(S/60)%60,bField?TEXT("antenna field"):TEXT("yard"));
  const float Roll=TestRoll>=0?TestRoll:FMath::FRand();TestRoll=-1;
  if(Roll<(bField?S47OpeningsLocal::ArrayKnockedOutChance:KnockedOutChance))
  {
   // Not at once: it gets a few seconds out first, which is worse.
   RingLeft=FMath::Min(RingLeft,5.);bDead=true;
   UE_LOG(LogTemp,Display,TEXT("S47: yard alarm will be put out"));
  }
  M->RequestAutosave();
 }
 if(RingLeft>0)
 {
  RingLeft-=Dt;HornIn-=Dt;
  if(HornIn<=0&&M->Audio){HornIn=1.1;M->Audio->Beep(420.f,.5f,.16f);M->Audio->Beep(560.f,.4f,.12f,.5f);}
  for(auto& L:Lamps)if(auto* C=L.Get()){C->SetVisibility(true);C->SetIntensity(6000.f);}
  if(RingLeft<=0){if(bDead&&M->UI)M->UI->Toast(TEXT("The horn stops in the middle of a note. The yard lights are out."),5);Show();}
 }
}
bool FS47YardAlarm::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s yard alarm: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 FS47YardAlarm A(nullptr);A.Begin(nullptr);Check(!A.Dead()&&A.Save()->Values.Num()==0,TEXT("an older save has none, and an alarm in order writes nothing"));
 auto O=S47J::New();S47J::Set(O,TEXT("dead"),true);FS47YardAlarm B(nullptr);B.Begin(S47J::Read(S47J::Write(O)));Check(B.Dead()&&S47J::Bool(B.Save(),TEXT("dead")),TEXT("put out stays put out after a Continue"));
 return OK;
}
