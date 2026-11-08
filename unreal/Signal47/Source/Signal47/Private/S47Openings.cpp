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
  {TEXT("spotlight"),TEXT("Roof spotlight for the truck"),35,TEXT("one branch of the array back in service, so the stores will talk to you"),TEXT("F in the cab. It reaches far and it can be seen from far. Whatever is out there sees the truck too.")},
  {TEXT("cot"),TEXT("Field cot in the truck"),20,TEXT("any field record kept"),TEXT("Sleep where you are, at any place on your map. Nothing between you and what comes by: it will wake you, and it may still be there.")},
  {TEXT("magnetometer"),TEXT("Survey magnetometer, used"),40,TEXT("a place found out on the land, or marks at a site kept as a record"),TEXT("On foot it leans toward marks, gear and places you have not looked at, and it reads what you find. It says nothing about what they are.")},
  {TEXT("logger2"),TEXT("Second field logger and contact pickups"),45,TEXT("THE REPEATED INTERVAL concluded, with the first logger on loan"),TEXT("Makes the two-site test on VISITORS AROUND THE BASE possible: the roof and a field post at the same minute.")},
  {TEXT("film"),TEXT("Fast film and a better lens"),30,TEXT("a photograph with a body in it"),TEXT("A full extra roll, frames that keep their subject in worse light, and $5 more on each registered incident.")},
  // GEIGER-20261108: not a counter yet, a part of one. The player builds it at the bench (FS47Workshop "geiger", S47Happenings.h).
  {TEXT("geiger_tube"),TEXT("Geiger-Muller tube and a meter movement"),25,TEXT("any field record kept: a place, marks, or scorched ground"),TEXT("Parts, not a counter. Build one at the CALIBRATION / REPAIR bench round a recorder housing and a lead, on a dry cell. On foot it ticks; where something has burnt or pressed the ground it ticks faster.")},
  {TEXT("alarm"),TEXT("Yard lights and klaxon"),60,TEXT("the county on speaking terms with the station"),TEXT("Goes off at what comes within fifty yards of the station. Tells you, and stops nothing. What sets it off may put it out; a fuse at its box mends it.")},
 };
 return Table;
}
const FS47KitOffer* FS47Openings::Def(const FString& Id){return Defs().FindByPredicate([&](const FS47KitOffer& O){return Id==O.Id;});}
bool FS47Openings::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s openings: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 TSet<FString> Ids;bool bWords=true;int32 Total=0;
 for(const auto& O:Defs()){Ids.Add(O.Id);Total+=O.Price;bWords&=O.Price>0&&FCString::Strlen(O.Waits)>3&&FCString::Strlen(O.What)>10;}
 Check(Ids.Num()==Defs().Num()&&bWords,TEXT("each has a price, what it waits for, and what it does"));
 Check(Total<=300,TEXT("all of them together are about five days' full pay"));
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
 if(!M->Freeform->Consume(TEXT("fuse"))){if(M->UI)M->UI->Toast(TEXT("It wants a fuse, and you have none on you."),4);return;}
 bDead=false;if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.5f});
 if(M->UI)M->UI->Toast(TEXT("New fuse. The yard alarm's lamp comes back on."),4);
 UE_LOG(LogTemp,Display,TEXT("S47: yard alarm mended"));M->RequestAutosave();
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
 const bool bHere=BodyNear();
 const bool bTrip=(bHere&&!bNear)||bTestTrip;bNear=bHere;bTestTrip=false;
 if(bTrip&&!bDead&&F.HasPowerForReactiveLight())
 {
  RingLeft=RingSeconds;HornIn=0;
  const int32 S=int32(FMath::Fmod(F.Clock(),86400.));
  F.RecordEvidence(FString::Printf(TEXT("alarm-day%d"),F.Day()),FString::Printf(TEXT("The yard alarm went off at %02d:%02d. Something was within its reach. It does not say what."),S/3600,(S/60)%60));
  if(M->UI)M->UI->Toast(TEXT("The yard alarm is going off."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: yard alarm trips at %02d:%02d"),S/3600,(S/60)%60);
  const float Roll=TestRoll>=0?TestRoll:FMath::FRand();TestRoll=-1;
  if(Roll<KnockedOutChance)
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
  if(RingLeft<=0){if(bDead&&M->UI)M->UI->Toast(TEXT("The klaxon stops in the middle of a note. The yard lights are out."),5);Show();}
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
