#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Radio.h"
#include "S47RoadGlimpse.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47TripoNew.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47FieldRadio.h"
#include "S47Keys.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HappenDev,Log,All);

// F10, "Night events" (TODO NIGHT31-20261108 and GEIGER-20261108): each of S47Happenings' events from its own line, with the clock and
// the place set so it can be seen, and a word on whether it really started. -S47DevSweep=/tmp/dir -S47DevSweepGroup="Night events" runs
// them all in turn, for a log.
namespace S47HapDev
{
 FVector2D ControlRoom(){return FVector2D(0,-1.2);}
 FVector2D Yard(){return FVector2D(7,19);}
 // A place of the catalogue in the given area's terms, or a stand-by spot when the catalogue does not have it.
 FVector2D CatalogSpot(US47Main* M,const TCHAR* Region,const TCHAR* IdPart,const FString& Area,const FVector2D& Otherwise,double Off)
 {
  if(M&&M->Freeform&&M->Freeform->EventPlaces)
   for(const FS47EventPlace& E:M->Freeform->EventPlaces->All())
    if(E.Region==Region||E.Id.Contains(IdPart))
    {
     const FVector W=S47OpenLand::FromRoad(Area,E.EntryX,E.EntryZ+Off);
     return FVector2D(W.X,W.Z);
    }
  return Otherwise;
 }
}

void FS47DevFreeform::AddHappenings(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Night events");
 const auto Haps=[this]()->FS47Happenings*{auto* F=Session();return F&&F->Happenings?F->Happenings.Get():nullptr;};
 // In the truck on the highway, north-bound and at speed, then Then. bKeep: the throttle stays on.
 const auto Road=[this](double From,double To,bool bKeep,TFunction<void()> Then)
 {
  if(!Session()||!M->World){Tell(false,TEXT("Freeform only"));return;}
  ClearRunning();ToHour(From,To);
  const double StartX=1.8,StartZ=1500.;
  FS47TripOpts O;O.AtPos=FVector(8000+StartX,S47RoadTerrain::SurfaceY(StartX,StartZ),StartZ);O.AtHeading=0.;O.Leg=TEXT("road");
  M->World->StartTrip(TEXT("road"),TEXT("diner"),O);
  M->World->TestInput=[this]{FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;if(D)I.Throttle=D->Speed<15?1:0;return I;};
  WaitFor([this]{auto* D=M->World?M->World->CabDrive():nullptr;return D&&M->World->bDriving&&!M->World->bBusy&&D->Speed>9;},14,
   [this,bKeep,Then]{if(!bKeep&&M->World)M->World->TestInput=nullptr;Then();},
   [this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck did not get going on the road"));});
 };
 // In the truck on the highway, then stopped and out on foot beside it (the truck parked on the road), then Then.
 const auto OutOfTruck=[this,Road](double From,TFunction<void()> Then)
 {
  Road(From,4.5,false,[this,Then]
  {
   if(M->World)M->World->TestInput=[this]{FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;I.Throttle=D&&D->Speed>.5?-1:0;return I;};
   WaitFor([this]{auto* D=M->World?M->World->CabDrive():nullptr;return !D||FMath::Abs(D->Speed)<.4;},14,[this,Then]
   {
    if(M->World)M->World->TestInput=nullptr;
    if(!M->World||!(M->World->GetOutHere()||M->World->GetOutHere(true))){Tell(false,TEXT("could not get out of the truck here"));return;}
    WaitFor([this]{return M->World&&!M->World->bDriving&&!M->World->bBusy&&M->World->TruckHere();},10,Then,[this]{Tell(false,TEXT("the truck did not let the player out"));});
   },[this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck did not stop"));});
  });
 };
 // To a lasting one out on the land, on foot, a few metres off it, looking at it.
 const auto ToSite=[this,Haps](const FString& Kind,double Off,const FString& Say)
 {
  FS47Happenings* H=Haps();
  if(!H||!H->HasSite(Kind)){Tell(false,FString::Printf(TEXT("no %s stands out there"),*Kind));return;}
  FStage Stage;Stage.From=22;Stage.To=4.5;Stage.Area=TEXT("road");Stage.Yaw=0;
  Stage.Where=[Haps,Kind,Off]{FVector At;FS47Happenings* Hs=Haps();if(Hs&&Hs->SiteHere(Kind,At))return FVector2D(At.X,At.Z+Off);return FVector2D(8000,0);};
  Go(Stage,[this,Haps,Kind,Say]
  {
   FS47Happenings* Hn=Haps();FVector At;
   if(!Hn||!Hn->SiteHere(Kind,At)){Tell(false,FString::Printf(TEXT("the %s is not in this area"),*Kind));return;}
   AimAt(At+FVector(0,.8,0));
   Tell(true,Say,6);
  });
 };

 // IGNITION-20261111 (Tom, 11 November): the key off and on in the cab, then out with it running: it idles where it stands, lights on.
 E.AddFreeform(TEXT("Truck (ignition)"),TEXT("Ignition and idling"),TEXT("22:00, in the truck on the highway: stop, key off and on, get out with it running"),[this,Road]
 {
  Road(22,4.5,false,[this]
  {
   if(M->World)M->World->TestInput=[this]{FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;I.Throttle=D&&D->Speed>.5?-1:0;return I;};
   WaitFor([this]{auto* D=M->World?M->World->CabDrive():nullptr;return !D||FMath::Abs(D->Speed)<.4;},14,[this]
   {
    US47World* W=M->World;if(!W)return;W->TestInput=nullptr;
    const FS47DriveController* D=W->CabDrive();const FS47Truck* T=W->CabTruck();
    if(!D||!T){Tell(false,TEXT("not in the truck"));return;}
    W->CabIgnition();const bool bOff=!D->bEngine&&T->Beam()<.01;
    W->CabIgnition();const bool bOn=D->bEngine&&T->Beam()>.5;
    if(!bOff||!bOn){UE_LOG(LogS47HappenDev,Error,TEXT("S47: ignition check FAIL: off %d on %d"),bOff,bOn);Tell(false,TEXT("the key did not turn the engine and lights off and on"));return;}
    if(!W->CabGetOut()){Tell(false,TEXT("could not get out of the truck here"));return;}
    WaitFor([this]{return M->World&&!M->World->bDriving&&!M->World->bBusy;},10,[this]
    {
     const bool bIdle=M->World->EngineIdling()&&M->World->IdleBeam()>.5;
     UE_LOG(LogS47HappenDev,Display,TEXT("S47: ignition check %s: key off/on, out with it idling and lit"),bIdle?TEXT("PASS"):TEXT("FAIL"));
     Tell(bIdle,bIdle?TEXT("Out of the truck with the engine running: it idles, headlights on. Walk off and it fades; get in and K turns it off."):TEXT("the truck did not stay running and lit"),8);
    },[this]{Tell(false,TEXT("the truck did not let the player out"));});
   },[this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck did not stop"));});
  });
 });
 E.AddFreeform(G,TEXT("Car dies: something goes over"),TEXT("01:30, in the truck at speed on the highway; lights, dash and radio out for seconds"),[this,Road,Haps]
 {
  Road(1.5,4.5,true,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("car_dies"),FString(),Why)||!H->Live(TEXT("car_dies"))){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   Tell(true,TEXT("Something goes over in a moment: the lights, the dash and the radio die for a few seconds. Keep driving."),6);
   After(10,[this,Haps]
   {
    auto* F=Session();FS47Happenings* Ha=Haps();
    if(M->World)M->World->TestInput=nullptr;
    const bool bKept=F&&F->HasFinding(FString::Printf(TEXT("car_dies_d%d"),F->Day()));
    Tell(bKept,bKept?FString::Printf(TEXT("The lights came back and the record is kept%s."),Ha&&Ha->DashAhead()>0?TEXT("; the dash clock now runs forty minutes ahead"):TEXT("")):FString(TEXT("it ran, but no record was kept")),5);
   });
  });
 });
 E.AddFreeform(G,TEXT("Dash clock: forty minutes ahead"),TEXT("21:00, in the truck; until the morning"),[this,Road,Haps]
 {
  Road(21,4.5,false,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("clock_ahead"),FString(),Why)){Tell(false,Why);return;}
   const FString Usual=M->Story?M->Story->ClockText(false):FString();
   Tell(H->DashAhead()>0,FString::Printf(TEXT("The dash now reads %s against the station's %s. It agrees again in the morning."),*FS47Happenings::DashClock(M,Usual),*Usual),6);
  });
 });
 E.AddFreeform(G,TEXT("Cattle standing in a ring"),TEXT("22:00, on foot out by the windmill, looking at them; they face the last landing if there is one"),[this,Haps]
 {
  ToLandPlace(TEXT("windmill"),60,22,4.5,[this,Haps](double,double)
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("cattle_ring"),FString(),Why)){Tell(false,Why);return;}
   FVector At;if(H->SiteHere(TEXT("cattle"),At))AimAt(At+FVector(0,1.,0));
   Tell(H->Live(TEXT("cattle_ring")),TEXT("Cattle ahead of you in a ring, every head the same way. Walk up: E on one keeps it as a record."),6);
  });
 });
 for(const TCHAR* Variant:{TEXT("real"),TEXT("empty"),TEXT("wrong")})
 {
  const FString V=Variant;
  const FString Label=V==TEXT("real")?TEXT("CB: a driver stuck far out"):V==TEXT("empty")?TEXT("CB: a driver stuck far out, and his rig empty"):TEXT("CB: a driver stuck far out, and the voice not right");
  // CB-TRUCK-20261109: the CB is under the truck's dash, so the call comes in the truck on the highway.
  E.AddFreeform(G,Label,TEXT("21:00, in the truck on the highway, on the CB under the dash; where he is goes on the map"),[this,Haps,V,Road]
  {
   Road(21,24,false,[this,Haps,V]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(TEXT("cb_stranded"),V,Why)){Tell(false,Why);return;}
    Tell(H->HasSite(TEXT("truck")),FString::Printf(TEXT("The call comes on the CB under the dash. Where he says he is, is on your map (%s)%s."),*S47Keys::MapSaid(),V==TEXT("wrong")?TEXT("; a later night the voice may come back"):TEXT("")),7);
   });
  });
 }
 E.AddFreeform(G,TEXT("Go: the stuck driver's rig"),TEXT("on foot beside it, at night; E on it. A new real call first, so there is someone to help"),[this,Haps,ToSite]
 {
  FS47Happenings* H=Haps();FString Why;
  if(!H||!H->DevStart(TEXT("cb_stranded"),TEXT("real"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  ToSite(TEXT("truck"),10,TEXT("His rig, ahead of you, and him beside it. E on it."));
 });
 E.AddFreeform(G,TEXT("CB: the voice says what you did"),TEXT("02:30, in the truck on the highway; a danger, after a voice that was not right"),[this,Haps,Road]
 {
  Road(2.5,4.5,false,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("cb_echo"),FString(),Why)||!H->Live(TEXT("cb_echo"))){Tell(false,Why);return;}
   Tell(true,TEXT("The CB under the dash opens by itself, and the voice says something you did."),6);
  });
 });
 E.AddFreeform(G,TEXT("Landing: lights come down, from the yard"),TEXT("23:00, yard, looking at them; the bearing goes on the map"),[this,Haps]
 {
  FStage Stage;Stage.From=23;Stage.To=4.5;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("landing"),FString(),Why)||!H->Live(TEXT("landing"))){Tell(false,Why);return;}
   FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,90,0));
   Tell(true,TEXT("Keep looking out there: first the crickets stop, then lights come down, and in half a minute the bearing is on your map."),7);
   After(38,[this,Haps]
   {
    auto* F=Session();FS47Happenings* Ha=Haps();
    const bool bKept=F&&F->HasFinding(FString::Printf(TEXT("landing_seen_d%d"),F->Day()));
    Tell(bKept&&Ha&&Ha->HasSite(TEXT("landing")),bKept?FString(TEXT("They came down and went out; the bearing is kept, and the ring is out there.")):FString(TEXT("they came down, but it was not seen or logged")),5);
   });
  });
 });
 E.AddFreeform(G,TEXT("Go: the scorched ring"),TEXT("on foot beside it, at night; E on it. Makes one first if there is none"),[this,Haps,ToSite]
 {
  FS47Happenings* H=Haps();FString Why;
  if(H&&!H->HasSite(TEXT("landing"))&&!H->DevStart(TEXT("landing"),TEXT("ring"),Why)){Tell(false,Why);return;}
  ToSite(TEXT("landing"),9,TEXT("Scorched ground ahead of you, four marks in it. E on it."));
 });
 E.AddFreeform(G,TEXT("Hitchhiker: a woman by the road"),TEXT("02:30, in the truck on the highway; she is on the right ahead. Stop for her, or drive past"),[this,Road,Haps]
 {
  Road(2.5,4.5,false,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("hitchhiker"),FString(),Why)||!H->Live(TEXT("hitchhiker"))){Tell(false,Why);return;}
   Tell(true,TEXT("A woman on the right-hand shoulder ahead, her hand out. Stop beside her, or drive past. The wheel is yours."),7);
  });
 });
 E.AddFreeform(G,TEXT("Hitchhiker: drive past her"),TEXT("02:30, the truck keeps going; she is gone in the mirror"),[this,Road,Haps]
 {
  Road(2.5,4.5,true,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("hitchhiker"),FString(),Why)||!H->Live(TEXT("hitchhiker"))){Tell(false,Why);return;}
   Tell(true,TEXT("Driving past the woman by the road. Watch the mirror."),6);
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return !Hw||!Hw->Live(TEXT("hitchhiker"));},40,[this]
   {
    auto* F=Session();if(M->World)M->World->TestInput=nullptr;
    const bool bKept=F&&F->HasFinding(FString::Printf(TEXT("hitcher_d%d"),F->Day()));
    Tell(bKept,bKept?FString(TEXT("Passed her; in the mirror the shoulder was empty. The record is kept.")):FString(TEXT("she went without a record")),5);
   },[this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck never passed her"));});
  });
 });
 // The danger on foot (Tom's critics, 8 November): out by the windmill at three, the night's own rules, the lamp on. Turn to it
 // when it is there; it comes nearer each time; run, or get under a roof, before the light comes down.
 E.AddFreeform(G,TEXT("On foot: something that comes nearer (danger)"),TEXT("03:00, on foot by the windmill, lamp on; it is gone when seen and nearer the next time, then a light comes down"),[this,Haps]
 {
  ToLandPlace(TEXT("windmill"),40,3,4.5,[this,Haps](double,double)
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("figure_near"),FString(),Why)||!H->Live(TEXT("figure_near"))){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,1.4,0));
   Tell(true,TEXT("Out there, ahead: a figure. Look at it, and look away. Get to the truck or under a roof, or run, before the light comes down."),7);
  });
 });
 E.AddFreeform(G,TEXT("Stranded camper: go to it"),TEXT("on foot beside it, at night, every window lit; E on the door"),[this,Haps,ToSite]
 {
  FS47Happenings* H=Haps();FString Why;
  if(H&&!H->HasSite(TEXT("rv"))&&!H->DevStart(TEXT("stranded_rv"),FString(),Why)){Tell(false,Why);return;}
  ToSite(TEXT("rv"),16,TEXT("The camper, every light on. E on the door."));
 });
 // Humour, once a week in play (never on a night with a danger): here, now.
 const auto Humor=[this,Haps](const FString& Kind,const FString& Say)
 {
  FS47Happenings* H=Haps();FString Why;
  if(!H||!H->DevStart(TEXT("humor"),Kind,Why)||!H->Live(TEXT("humor"))){Tell(false,Why);return;}
  FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,1.2,0));
  Tell(true,Say,6);
 };
 E.AddFreeform(G,TEXT("Humour: a pink convertible at the diner"),TEXT("21:30, the diner's lot; engine running, a note on the seat; E on it"),[this,Humor]
 {
  FStage Stage;Stage.From=21.5;Stage.To=1;Stage.Area=TEXT("diner");Stage.Yaw=0;
  Stage.Where=[this]{return S47HapDev::CatalogSpot(M,TEXT("diner"),TEXT("diner"),TEXT("diner"),FVector2D(7978,1972),-10);};
  Go(Stage,[Humor]{Humor(TEXT("car"),TEXT("A pink convertible, running, a note on the seat. E on it."));});
 });
 E.AddFreeform(G,TEXT("Humour: a singer by the motel pool"),TEXT("22:00, the motel; he waves; E listens"),[this,Humor]
 {
  FStage Stage;Stage.From=22;Stage.To=1;Stage.Area=TEXT("saro");Stage.Yaw=0;
  Stage.Where=[]{const FS47Box2& Park=S47Legs::MOTEL_PARK();return FVector2D(Park.MinX-6,Park.MinZ+6);};
  Go(Stage,[Humor]{Humor(TEXT("singer"),TEXT("A man in a white suit singing to the empty pool. E listens."));});
 });
 E.AddFreeform(G,TEXT("Humour: something crosses the road"),TEXT("01:00, in the truck on the highway, in the lights"),[this,Road,Haps]
 {
  Road(1,4.5,false,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("humor"),TEXT("crossing"),Why)||!H->Live(TEXT("humor"))){Tell(false,Why);return;}
   Tell(true,TEXT("Something tall and hairy is about to cross in your lights."),5);
  });
 });
 // GLIMPSE-ROAD-20261109: the glimpse tier's crossing far ahead in the lights (S47RoadGlimpse): Tom's Cosmic or Nordic when this build
 // has them, else the tall stand-in; on four, the crawler. Gone before the truck comes up; some nights prints on the shoulder.
 for(const TCHAR* Body:{TEXT("tall"),TEXT("low")})
 {
  const FString Want=Body;
  E.AddFreeform(G,Want==TEXT("tall")?TEXT("Glimpse: something tall crosses far ahead"):TEXT("Glimpse: something low crosses far ahead"),TEXT("00:30, in the truck on the highway; 60 to 120 m ahead in the lights, gone before you get there"),[this,Road,Haps,Want]
  {
   Road(.5,4.5,false,[this,Haps,Want]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(TEXT("road_glimpse"),Want,Why)||!H->Live(TEXT("road_glimpse"))){Tell(false,Why);return;}
    Tell(true,TEXT("Watch the road far ahead, in the lights."),4);
   });
  });
 }
 // F10-NORDIC-COSMIC-20261109 (Tom, 9 November: «Nordic og Cosmic alien må inn i F10-dev-menyen også»): his Nordic and Cosmic one by
 // name in each event that may have them (S47RoadGlimpse::DevBodies): crossing the road at 00:30 and on foot at 02:30 here, on the rise
 // at 00:30 with the rest below. Each asks the event for its body (its own Variant) and says which it got; without Tom's private
 // models (a build from Git alone) it gets what stands in, says so and passes.
 const auto Body=[this,Haps](const FString& Id,const FString& Want,bool bAim)
 {
  FS47Happenings* H=Haps();FString Why;
  if(!H||!H->DevStart(Id,Want,Why)||!H->Live(Id)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  if(bAim){FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,1.4,0));}
  const FString Got=H->LiveVariant();
  const TCHAR* Own=S47RoadGlimpse::PathOf(Want);
  const bool bHave=Own&&S47TripoNew::Exists(Own);
  const FString Line=FString::Printf(TEXT("%s asked for %s: body %s (%s)%s"),*Id,*Want,Got.IsEmpty()?TEXT("shade"):*Got,S47RoadGlimpse::DevBodyName(Got),
   bHave?TEXT(""):TEXT("; Tom's model is not in this build, what stands in for it is there"));
  UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 alien body %s"),*Line);
  Tell(bHave?Got==Want:Got!=Want,Line,6);
 };
 for(const S47RoadGlimpse::FDevBody& D:S47RoadGlimpse::DevBodies())
 {
  const FString Id=D.Event,Want=D.Body;const double Hour=D.Hour;
  if(Id==TEXT("road_glimpse"))
   E.AddFreeform(G,D.Label,D.Note,[Road,Body,Id,Want,Hour]{Road(Hour,4.5,false,[Body,Id,Want]{Body(Id,Want,false);});});
  else if(Id==TEXT("figure_near"))
   E.AddFreeform(G,D.Label,D.Note,[this,Body,Id,Want,Hour]{ToLandPlace(TEXT("windmill"),40,Hour,4.5,[Body,Id,Want](double,double){Body(Id,Want,true);});});
 }
 E.AddFreeform(G,TEXT("Humour: a small man at the pump wants water"),TEXT("22:00, the pump in Caliche; E talks to him"),[this,Humor]
 {
  FStage Stage;Stage.From=22;Stage.To=1;Stage.Area=TEXT("diner");Stage.Yaw=0;
  Stage.Where=[this]
  {
   auto* F=Session();
   const FVector Gas=F&&F->Director?F->Director->LightPlacePoint(TEXT("gas")):FVector::ZeroVector;
   return S47HapDev::CatalogSpot(M,TEXT("__none"),TEXT("gas"),TEXT("diner"),Gas.IsZero()?FVector2D(7995,2250):FVector2D(Gas.X,Gas.Z+8),8);
  };
  Go(Stage,[Humor]{Humor(TEXT("pump"),TEXT("A small gray man in a shirt and a hat, at the pump. E talks to him."));});
 });
 // ---- the rest of NATTHENDELSER-31 ----
 static const TCHAR* const R=TEXT("Night events: the rest");
 struct FRest{const TCHAR* Id;const TCHAR* Label;const TCHAR* Note;int32 Stage;const TCHAR* Site;};
 // Stage: 0 out by the windmill on foot, 1 Caliche's street, 2 the control room, 3 the yard by the parked truck, 4 the highway, 5 the motel lot.
 static const FRest Rest[]={
  {TEXT("dogs_hush"),TEXT("2. Every dog barks, then all stop"),TEXT("23:00, on foot near the windmill"),0,nullptr},
  {TEXT("phone_empty"),TEXT("6. A phone rings in an empty building"),TEXT("23:00, Caliche; E on it answers"),1,nullptr},
  {TEXT("carcass"),TEXT("10. A dead cow with clean cuts: go to it"),TEXT("on foot beside it; E on it"),-1,TEXT("carcass")},
  {TEXT("house_lit"),TEXT("12. A light in an empty house"),TEXT("22:00, Caliche; it goes out when you come up"),1,TEXT("houselit")},
  {TEXT("horizon_figure"),TEXT("14. A figure on the rise"),TEXT("23:00, on foot near the windmill; the lamp finds nobody"),0,nullptr},
  {TEXT("cinema_film"),TEXT("15. The cinema runs a film for nobody"),TEXT("22:00, Caliche, by the Plaza"),1,TEXT("cinema")},
  {TEXT("room6_tv"),TEXT("16. Room 6: the door open a crack, the television on"),TEXT("23:00, the motel lot"),5,nullptr},
  {TEXT("town_blackout"),TEXT("17. Caliche goes dark for a minute"),TEXT("22:00, Caliche's street"),1,nullptr},
  {TEXT("array_burst"),TEXT("18. The array catches a burst: a bearing"),TEXT("23:00, control room; the bearing goes on the map"),2,nullptr},
  {TEXT("am_caller"),TEXT("19. 1420: a caller tells last night wrong"),TEXT("21:00, in the truck, when it is the caller's turn on the program"),4,nullptr},
  {TEXT("hood_warm"),TEXT("20. The truck's hood is warm"),TEXT("23:00, the yard by the truck"),3,nullptr},
  {TEXT("truck_tracks"),TEXT("21. Prints round the truck that stop"),TEXT("23:00, the yard by the truck"),3,TEXT("tracks")},
  {TEXT("animals_flee"),TEXT("22. Jackrabbits run past, all one way"),TEXT("23:00, on foot near the windmill"),0,nullptr},
  {TEXT("car_light"),TEXT("24. A light over the truck; the engine dies (danger)"),TEXT("02:30, the highway; stay in, or get out under it"),4,nullptr},
  {TEXT("dust_devil"),TEXT("25. A dust devil that stands still"),TEXT("21:00, on foot near the windmill"),0,nullptr},
  {TEXT("missing_person"),TEXT("26. The missing man's pickup: go to it"),TEXT("the county on the CB, then on foot beside it; E on it"),-1,TEXT("missing")},
 };
 for(const FRest& Line:Rest)
 {
  const FString Id=Line.Id;const int32 St=Line.Stage;const FString Site=Line.Site?Line.Site:TEXT("");
  E.AddFreeform(R,Line.Label,Line.Note,[this,Haps,Road,OutOfTruck,ToSite,Id,St,Site]
  {
   const auto Fire=[this,Haps,Id,Site]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(Id,FString(),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
    FVector At;
    if(H->LiveAt(At))AimAt(At+FVector(0,1.2,0));else if(!Site.IsEmpty()&&H->SiteHere(Site,At))AimAt(At+FVector(0,1.6,0));
    Tell(H->Live(Id)||Id==TEXT("hood_warm")||Id==TEXT("am_caller")||Id==TEXT("town_blackout"),FString::Printf(TEXT("%s: started."),*Id),5);
   };
   if(St<0)
   {
    // Out there somewhere: start it, then go and stand by it.
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(Id,FString(),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
    ToSite(Site,9,FString::Printf(TEXT("%s, ahead of you. E on it."),*Id));
    return;
   }
   // The caller waits for the program's turn: asked again until it comes, for half a minute.
   if(Id==TEXT("am_caller"))
   {
    Road(21,4.5,false,[this,Haps]
    {
     // On 1420, so that the program runs where it can give her the turn.
     if(M->CarRadio.IsValid())M->CarRadio->Set(true,TEXT("caliche"));
     WaitFor([Haps]{FS47Happenings* Hw=Haps();FString Why;return Hw&&Hw->DevStart(TEXT("am_caller"),FString(),Why);},75,[this]{Tell(true,TEXT("am_caller: started, when the program gave the caller her turn."),5);},[this]{Tell(false,TEXT("the program never gave the caller her turn"));});
    });
    return;
   }
   if(St==4){Road(Id==TEXT("car_light")?2.5:21,4.5,false,Fire);return;}
   // By the truck: parked out on the road and the player out of it.
   if(St==3){OutOfTruck(23,Fire);return;}
   if(St==0){ToLandPlace(TEXT("windmill"),40,Id==TEXT("dust_devil")?21:23,4.5,[Fire](double,double){Fire();});return;}
   FStage Stage;Stage.To=4.5;Stage.Yaw=UE_DOUBLE_PI;
   if(St==1){Stage.From=Id==TEXT("phone_empty")?23:22;Stage.Area=TEXT("diner");Stage.Yaw=0;Stage.Where=[]{return FVector2D(7995,2110);};}
   else if(St==2){Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};}
   else if(St==3){Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};}
   else{Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{const FS47Box2& Park=S47Legs::MOTEL_PARK();return FVector2D(Park.MinX-6,Park.MinZ+6);};}
   Go(Stage,Fire);
  });
 }
 // F10-NORDIC-COSMIC-20261109: the figure on the rise as the Nordic or the Cosmic one (S47RoadGlimpse::DevBodies), at 00:30.
 for(const S47RoadGlimpse::FDevBody& D:S47RoadGlimpse::DevBodies())
 {
  const FString Id=D.Event,Want=D.Body;const double Hour=D.Hour;
  if(Id==TEXT("horizon_figure"))
   E.AddFreeform(R,D.Label,D.Note,[this,Body,Id,Want,Hour]{ToLandPlace(TEXT("windmill"),40,Hour,4.5,[Body,Id,Want](double,double){Body(Id,Want,true);});});
 }
 // Without F10's shortcut: the light over the truck takes a player who gets out under it, in the real game's rules, and the hood is
 // warm for one who walks away from the truck and comes back.
 E.AddFreeform(R,TEXT("20b. The hood, walked back to"),TEXT("the truck parked on the road; away fifty metres for twenty-five minutes, back to it, and the night itself asked"),[this,Haps,OutOfTruck]
 {
  OutOfTruck(23,[this,Haps]
  {
   auto* F=Session();FS47Truck* T=M->World?M->World->TruckHere():nullptr;
   if(!F||!T||!M->Player){Tell(false,TEXT("no truck parked here"));return;}
   const FVector At=T->Position;
   M->Player->Place(At.X+50,At.Z,0);
   After(2,[this,Haps,At]
   {
    auto* Fa=Session();if(!Fa){Tell(false,TEXT("Freeform only"));return;}
    Fa->TestAdvance(25*60.);
    After(2,[this,Haps,At]
    {
     M->Player->Place(At.X+3,At.Z,UE_DOUBLE_PI/2);
     After(3,[this,Haps]
     {
      FS47Happenings* H=Haps();FString Why;
      const bool bOk=H&&H->DevTry(TEXT("hood_warm"),Why);
      UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 hood_warm walked back to: %s"),bOk?TEXT("warm"):*Why);
      Tell(bOk,bOk?FString(TEXT("Back at the truck after twenty-five minutes away: the hood is warm.")):Why,6);
     });
    });
   });
  });
 });
 E.AddFreeform(R,TEXT("24b. The light over the truck: get out under it"),TEXT("02:30, the highway, in the real game's rules; the light comes down over you (this test run keeps those rules on)"),[this,Haps,Road]
 {
  auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
  F->TestLiveRules();
  Road(2.5,4.5,false,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("car_light"),FString(),Why)){Tell(false,Why);return;}
   // The engine is dead under it: on the brake to a stop, then out.
   if(M->World)M->World->TestInput=[this]{FS47DriveIn Pedal;auto* D=M&&M->World?M->World->CabDrive():nullptr;Pedal.Throttle=D&&D->Speed>.3?-1:0;return Pedal;};
   WaitFor([this]{auto* D=M->World?M->World->CabDrive():nullptr;return !D||FMath::Abs(D->Speed)<.4;},18,[this]
   {
    if(M->World)M->World->TestInput=nullptr;
    if(!M->World||!(M->World->GetOutHere(true)||M->World->GetOutHere())){Tell(false,TEXT("could not get out under the light"));return;}
    WaitFor([this]{auto* Fs=Session();return Fs&&Fs->Taken&&Fs->Taken->Busy();},18,[this]
    {
     UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 car_light: out under the light, taken"));
     Tell(true,TEXT("Out under the light: it came down over you. Taken."),6);
    },[this]{Tell(false,TEXT("out under the light, and it did not come down"));});
   },[this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck never stopped under the light"));});
  });
 });
 // ---- unease (URO.md): each of the 25 where it can be noticed; nothing is kept and nothing is explained ----
 static const TCHAR* const U=TEXT("Unease");
 for(int32 I=0;I<FS47Happenings::UneaseCount();++I)
 {
  // Where it can happen: in the control room, out in the yard, at the wheel, or in Caliche's street.
  const int32 Where=(I==1||I==5||I==7||I==9||I==11||I==13||I==16||I==19)?0:(I==6||I==14||I==18||I==21||I==23)?2:I==22?3:1;
  static const TCHAR* const WhereText[]={TEXT("23:00, control room"),TEXT("23:00, the yard, on foot"),TEXT("23:00, in the truck on the highway"),TEXT("21:30, Caliche's street")};
  const FString Label=FString::Printf(TEXT("%d. %s"),I+1,FS47Happenings::UneaseTitle(I));
  E.AddFreeform(U,Label,WhereText[Where],[this,Haps,Road,I,Where]
  {
   const auto Fire=[this,Haps,I]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevUnease(I,Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
    Tell(true,FString::Printf(TEXT("%s. Nothing is kept, nothing is said."),FS47Happenings::UneaseTitle(I)),5);
   };
   if(Where==2){Road(23,4.5,false,Fire);return;}
   FStage Stage;Stage.From=Where==3?21.5:23;Stage.To=4.5;Stage.Yaw=UE_DOUBLE_PI;
   Stage.Area=Where==3?TEXT("diner"):TEXT("saro");
   Stage.Where=[Where]{return Where==0?S47HapDev::ControlRoom():Where==1?S47HapDev::Yard():FVector2D(7995,2120);};
   if(Where==3)Stage.Yaw=0;
   Go(Stage,Fire);
  });
 }
 E.AddFreeform(G,TEXT("Counter: give me one, held up"),TEXT("as if built at the bench; this test run only. It ticks on foot"),[this,Haps]
 {
  FS47Happenings* H=Haps();if(!H){Tell(false,TEXT("Freeform only"));return;}
  H->DevGeiger(true,true);
  Tell(H->GeigerBuilt(),TEXT("The counter is yours and held up: lower left. It ticks; near a ring, marks or a place a bearing found, faster."),6);
 });
 E.AddFreeform(G,TEXT("Counter: at the scorched ring"),TEXT("on foot in the middle of it, the counter up; it pegs. Makes a ring first if there is none"),[this,Haps]
 {
  FS47Happenings* H=Haps();FString Why;if(!H){Tell(false,TEXT("Freeform only"));return;}
  if(!H->HasSite(TEXT("landing"))&&!H->DevStart(TEXT("landing"),TEXT("ring"),Why)){Tell(false,Why);return;}
  H->DevGeiger(true,true);
  FStage Stage;Stage.From=22;Stage.To=4.5;Stage.Area=TEXT("road");Stage.Yaw=0;Stage.Pitch=-.5;
  Stage.Where=[Haps]{FVector At;FS47Happenings* Hs=Haps();if(Hs&&Hs->SiteHere(TEXT("landing"),At))return FVector2D(At.X+.5,At.Z+1.5);return FVector2D(8000,0);};
  Go(Stage,[this,Haps]
  {
   After(3,[this,Haps]
   {
    FS47Happenings* Ha=Haps();
    const float Read=Ha?Ha->GeigerReading():0.f;
    UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 counter at the ring reads %.2f, shown %d"),Read,Ha&&Ha->GeigerShown()?1:0);
    Tell(Read>.6f&&Ha&&Ha->GeigerShown(),FString::Printf(TEXT("In the ring the counter reads %.2f of full scale%s."),Read,Read>.85f?TEXT(": pegged"):TEXT("")),6);
   });
  });
 });
 // D45-20261110 (Tom, d45): a fresh ring smokes a little, crackles close to and has coals glowing at its edge; hours on they have cooled.
 for(const double Hours:{0.,4.})
 {
  E.AddFreeform(G,Hours>0?TEXT("Scorched ring: four hours on"):TEXT("Scorched ring: coals, smoke and crackle"),
   Hours>0?TEXT("the same ring as it is four hours after it came down: the smoke gone, the coals dull"):TEXT("23:00, on foot a few steps off a fresh ring, looking at it; makes one first if there is none"),[this,Haps,Hours]
  {
   FS47Happenings* H=Haps();FString Why;if(!H){Tell(false,TEXT("Freeform only"));return;}
   if((Hours<=0||!H->HasSite(TEXT("landing")))&&!H->DevStart(TEXT("landing"),TEXT("ring"),Why)){Tell(false,Why);return;}
   if(Hours>0)H->DevAgeRing(Hours);
   FStage Stage;Stage.From=23;Stage.To=4.5;Stage.Area=TEXT("road");Stage.Yaw=0;Stage.Pitch=-.3;
   Stage.Where=[Haps]{FVector At;FS47Happenings* Hs=Haps();if(Hs&&Hs->SiteHere(TEXT("landing"),At))return FVector2D(At.X,At.Z+7);return FVector2D(8000,0);};
   Go(Stage,[this,Haps,Hours]
   {
    After(4,[this,Haps,Hours]
    {
     FS47Happenings* Ha=Haps();FString Said;
     const bool bShown=Ha&&Ha->DevEmbers(Said);
     UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 scorched ring: %s"),*Said);
     // Four hours on the coals are still there, dull, and the smoke is done; fresh, it smokes and crackles.
     Tell(bShown&&(Hours>0?Said.Contains(TEXT("smoke 0.00")):Said.Contains(TEXT("crackle on"))&&!Said.Contains(TEXT("smoke 0.00"))),Said,7);
    });
   });
  });
 }
 // ---- M7: the first big night (S47BigNight.cpp), each shape where it can be met, and the day after ----
 // -S47DevSweep=/tmp/dir "-S47DevSweepGroup=First big night (M7)" runs it in turn; its own pictures are bignight-<what>.png beside the
 // sweep's. The natural line comes first: it needs a game that has not reached night 2 or 3 yet.
 static const TCHAR* const B=TEXT("First big night (M7)");
 const auto Shot=[](const FString& Name)
 {
  FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("S47DevSweep="),Dir)||Dir.IsEmpty())return;
  FScreenshotRequest::RequestScreenshot(Dir/(TEXT("bignight-")+Name+TEXT(".png")),false,false);
  UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 big night picture %s"),*Name);
 };
 // What came of it, for the log and the screen: the records kept and what followed.
 const auto Report=[this,Haps](const FString& What)->FString
 {
  auto* F=Session();FS47Happenings* H=Haps();if(!F||!H)return FString(TEXT("Freeform only"));
  const int32 N=H->BigHappened();
  FString Kept;
  for(const FString& K:{FString::Printf(TEXT("array_burst_d%d_turn"),N),FString::Printf(TEXT("landing_seen_d%d_wide"),N),FString::Printf(TEXT("big_felt_d%d"),N),FString::Printf(TEXT("lights_trace_bignight_d%d"),N)})if(F->HasFinding(K))Kept+=K+TEXT(" ");
  FString Follow;for(const TCHAR* W:{TEXT("array"),TEXT("seen"),TEXT("felt"),TEXT("dale"),TEXT("slip"),TEXT("radio"),TEXT("cb"),TEXT("trace"),TEXT("unseen")})if(H->BigHas(W))Follow+=FString(W)+TEXT(" ");
  const FString Line=FString::Printf(TEXT("%s: night %d, bearing %03.0f, %.0f up, out past %s; kept %s; followed %s"),*What,N,H->BigBearing(),H->BigElevation(),*H->BigPlaceTitle(),Kept.IsEmpty()?TEXT("nothing"):*Kept,Follow.IsEmpty()?TEXT("nothing"):*Follow);
  UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 big night %s"),*Line);
  return Line;
 };
 // What a run must have left to pass: Need, each of these followed ("array", "seen|felt": either).
 const auto Left=[Haps](std::initializer_list<const TCHAR*> Need)
 {
  FS47Happenings* H=Haps();if(!H||H->BigHappened()<=0)return false;
  for(const TCHAR* N:Need){bool bAny=false;TArray<FString> Or;FString(N).ParseIntoArray(Or,TEXT("|"));for(const FString& W:Or)bAny|=H->BigHas(W);if(!bAny)return false;}
  return true;
 };
 // The middle of the array, a little above the dishes, to look at it turn.
 const auto ArrayMiddle=[this]()->FVector
 {
  FVector Sum=FVector::ZeroVector;int32 N=0;
  if(M->Web)for(int32 I=0;I<M->Web->DishCount();++I){Sum+=FVector(M->Web->DishX(I),0,M->Web->DishZ(I));++N;}
  const double Y=M->Player?M->Player->FloorY+6.:6.;
  return N>0?FVector(Sum.X/N,Y,Sum.Z/N):FVector(0,Y,-120);
 };
 E.AddFreeform(B,TEXT("Plan: this game's night, hour and shape"),TEXT("from the seed; once in a game, night 2 or 3, after two"),[this,Haps]
 {
  FS47Happenings* H=Haps();if(!H){Tell(false,TEXT("Freeform only"));return;}
  const FS47Happenings::FBigPlan P=H->BigPlan();
  FString Seeds;
  for(int32 Seed=1;Seed<=8;++Seed){const FS47Happenings::FBigPlan Q=FS47Happenings::BigPlanFor(Seed);Seeds+=FString::Printf(TEXT(" seed %d: night %d %02d:%02d %s;"),Seed,Q.Night,int32(Q.Hour)-24,int32(FMath::Fmod(Q.Hour,1.)*60.),FS47Happenings::BigVariantName(Q.Variant));}
  UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 big night plan: night %d %02d:%02d %s, happened %d;%s"),P.Night,int32(P.Hour)-24,int32(FMath::Fmod(P.Hour,1.)*60.),FS47Happenings::BigVariantName(P.Variant),H->BigHappened(),*Seeds);
  Tell(((P.Night>=2&&P.Night<=3)||(P.Night>=9&&P.Night<=12)||(P.Night>=16&&P.Night<=19))&&P.Hour>=26,FString::Printf(TEXT("This game: night %d from %02d:%02d, the %s. Happened on night %d (0: not yet)."),P.Night,int32(P.Hour)-24,int32(FMath::Fmod(P.Hour,1.)*60.),FS47Happenings::BigVariantName(P.Variant),H->BigHappened()),7);
 });
 E.AddFreeform(B,TEXT("Natural: the planner's own night and hour, from the yard"),TEXT("the clock to this game's night and hour, at SARO's yard; it starts by itself, or says why not"),[this,Haps,Shot,Report,Left]
 {
  FS47Happenings* H=Haps();auto* F=Session();if(!H||!F){Tell(false,TEXT("Freeform only"));return;}
  H->DevBigNatural();
  const FS47Happenings::FBigPlan P=H->BigPlan();
  if(F->Day()>P.Night||(F->Day()==P.Night&&FS47Happenings::NightHour(F->Clock())>P.Hour)){Tell(false,FString::Printf(TEXT("this game is past its night %d: start a new one for this line"),P.Night));return;}
  FStage Stage;Stage.Day=P.Night;Stage.From=P.Hour-24.;Stage.To=P.Hour-24.+.6;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps,Shot,Report,Left]
  {
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return Hw&&Hw->BigLive();},20,[this,Haps,Shot,Report,Left]
   {
    FVector At;FS47Happenings* Ha=Haps();if(Ha&&Ha->LiveAt(At))AimAt(At+FVector(0,220,0));
    After(30,[Shot]{Shot(TEXT("natural-climax"));});
    After(50,[this,Report,Left]{Tell(Left({TEXT("seen|felt")}),Report(TEXT("natural")),6);});
   },[this,Haps]{FS47Happenings* Hn=Haps();Tell(false,FString::Printf(TEXT("it did not start at its hour (happened on night %d)"),Hn?Hn->BigHappened():0));});
  });
 });
 for(const TCHAR* Shape:{TEXT("column"),TEXT("fall"),TEXT("rise")})
 {
  const FString V=Shape;
  E.AddFreeform(B,FString::Printf(TEXT("Big night (%s): from the yard at SARO"),Shape),TEXT("02:30, the yard; the array turns, then out there in its direction; pictures as it turns and at its height"),[this,Haps,Shot,Report,Left,ArrayMiddle,V]
  {
   FStage Stage;Stage.From=2.5;Stage.To=4.5;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Haps,Shot,Report,Left,ArrayMiddle,V]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(TEXT("big_night"),V,Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
    // The array first, as it turns; then out there.
    AimAt(ArrayMiddle());
    After(15,[Shot,V]{Shot(V+TEXT("-yard-turn"));});
    After(24,[this,Haps]{FVector At;FS47Happenings* Ha=Haps();if(Ha&&Ha->LiveAt(At))AimAt(At+FVector(0,250,0));});
    After(31,[Shot,V]{Shot(V+TEXT("-yard-climax"));});
    After(52,[this,Report,Left,V]{Tell(Left({TEXT("array"),TEXT("turned"),TEXT("seen|felt")}),Report(V+TEXT(" from the yard")),6);});
   });
  });
 }
 E.AddFreeform(B,TEXT("Big night (column): in the control room"),TEXT("02:30, the control room; the lights, the rack, the note, the dark, the printer"),[this,Haps,Shot,Report,Left]
 {
  FStage Stage;Stage.From=2.5;Stage.To=4.5;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps,Shot,Report,Left]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("column"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   // BIGNIGHT-SEEN-20261109: nobody can look out from in here, so its height comes at the latest, 40 s later than before.
   After(12,[Shot]{Shot(TEXT("column-room-flicker"));});
   After(70,[Shot]{Shot(TEXT("column-room-dark"));});
   After(87,[Shot]{Shot(TEXT("column-room-back"));});
   After(92,[this,Report,Left]{Tell(Left({TEXT("array"),TEXT("seen|felt")}),Report(TEXT("column in the control room")),6);});
  });
 });
 E.AddFreeform(B,TEXT("Big night (fall): in the truck on the highway"),TEXT("02:30, at the wheel north-bound; the lamps blink and die, and something comes down out there"),[this,Road,Haps,Shot,Report,Left]
 {
  Road(2.5,4.5,true,[this,Haps,Shot,Report,Left]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("fall"),Why)){if(M->World)M->World->TestInput=nullptr;Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   // BIGNIGHT-SEEN-20261109: at the wheel the eyes are on the road; its height comes when it is looked at, at the latest 40 s later.
   After(9,[Shot]{Shot(TEXT("fall-truck-blink"));});
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return Hw&&Hw->BigCameUp();},75,[this,Shot]{After(3,[Shot]{Shot(TEXT("fall-truck-climax"));});},[this]{Tell(false,TEXT("its height never came"));});
   After(92,[this,Report,Left]{if(M->World)M->World->TestInput=nullptr;Tell(Left({TEXT("seen|felt")}),Report(TEXT("fall in the truck")),6);});
  });
 });
 E.AddFreeform(B,TEXT("Big night (rise): on foot by the windmill"),TEXT("02:30, out on the land with the lamp; it flickers, and lights go up out there"),[this,Haps,Shot,Report,Left]
 {
  ToLandPlace(TEXT("windmill"),40,2.5,4.5,[this,Haps,Shot,Report,Left](double,double)
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("rise"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,200,0));
   After(14,[Shot]{Shot(TEXT("rise-foot-flicker"));});
   After(32,[Shot]{Shot(TEXT("rise-foot-climax"));});
   After(48,[this,Report,Left]{Tell(Left({TEXT("seen|felt")}),Report(TEXT("rise on foot")),6);});
  });
 });
 // BIGNIGHT-SEEN-20261109 (Tom, d37): the rise as it is now (soft light, the glow over the horizon first, its height waiting for the
 // eyes), from Caliche looking at it, from SARO looking away first, and at the wheel never looking.
 const auto Held=[Haps]()->FString{FS47Happenings* H=Haps();return H?FString::Printf(TEXT("; waited %.1f s for the eyes, came up %d"),H->BigHeldFor(),H->BigCameUp()?1:0):FString();};
 E.AddFreeform(B,TEXT("Big night: rise seen from Caliche"),TEXT("02:30, Caliche's street, looking at it: the glow over the horizon, then the lights go up in a soft column"),[this,Haps,Shot,Report,Left,Held]
 {
  FStage Stage;Stage.From=2.5;Stage.To=4.5;Stage.Area=TEXT("diner");Stage.Where=[]{return FVector2D(7995,2120);};Stage.Yaw=0;
  Go(Stage,[this,Haps,Shot,Report,Left,Held]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("rise"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   FVector At;if(H->LiveAt(At))AimAt(At+FVector(0,160,0));
   After(14,[Shot]{Shot(TEXT("rise-caliche-glow"));});
   After(31,[Shot]{Shot(TEXT("rise-caliche-climax"));});
   After(34,[this,Haps]{FVector At2;FS47Happenings* Ha=Haps();if(Ha&&Ha->LiveAt(At2))AimAt(At2+FVector(0,450,0));});
   After(35,[Shot]{Shot(TEXT("rise-caliche-high"));});
   After(54,[this,Haps,Report,Left,Held]{FS47Happenings* Hr=Haps();Tell(Left({TEXT("seen")})&&Hr&&Hr->BigHeldFor()<2,Report(TEXT("rise from Caliche"))+Held(),6);});
  });
 });
 E.AddFreeform(B,TEXT("Big night: rise from SARO"),TEXT("02:30, the yard, looking at the array; its height waits; at 40 s look its way: it comes up then"),[this,Haps,Shot,Report,Left,ArrayMiddle,Held]
 {
  FStage Stage;Stage.From=2.5;Stage.To=4.5;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps,Shot,Report,Left,ArrayMiddle,Held]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("rise"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   AimAt(ArrayMiddle());
   After(38,[this,Haps,Shot]{Shot(TEXT("rise-saro-waiting"));FS47Happenings* Hw=Haps();UE_LOG(LogS47HappenDev,Display,TEXT("S47: F10 big night from SARO: at 38 s, waited %.1f s, came up %d"),Hw?Hw->BigHeldFor():0.,Hw&&Hw->BigCameUp()?1:0);});
   After(40,[this,Haps]{FVector At;FS47Happenings* Ha=Haps();if(Ha&&Ha->LiveAt(At))AimAt(At+FVector(0,160,0));});
   After(46,[Shot]{Shot(TEXT("rise-saro-climax"));});
   After(75,[this,Haps,Report,Left,Held]{FS47Happenings* Hr=Haps();const double W=Hr?Hr->BigHeldFor():0;Tell(Left({TEXT("array"),TEXT("seen")})&&W>8&&W<20,Report(TEXT("rise from SARO"))+Held(),6);});
  });
 });
 E.AddFreeform(B,TEXT("Big night: rise while driving"),TEXT("02:30, at the wheel north-bound, never looking: the lamps blink, the radio breaks up, and its height comes at the latest"),[this,Road,Haps,Shot,Report,Left,Held]
 {
  Road(2.5,4.5,true,[this,Haps,Shot,Report,Left,Held]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("big_night"),TEXT("rise"),Why)){if(M->World)M->World->TestInput=nullptr;Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   After(2,[Shot]{Shot(TEXT("rise-drive-blink"));});
   After(16,[Shot]{Shot(TEXT("rise-drive-glow"));});
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return Hw&&Hw->BigCameUp();},80,[this,Shot]{After(3,[Shot]{Shot(TEXT("rise-drive-climax"));});},[this]{Tell(false,TEXT("its height never came"));});
   After(98,[this,Haps,Report,Left,Held]{if(M->World)M->World->TestInput=nullptr;FS47Happenings* Hr=Haps();Tell(Left({TEXT("seen|felt")})&&Hr&&(Hr->BigHas(TEXT("seen"))||Hr->BigHeldFor()>=39),Report(TEXT("rise while driving"))+Held(),6);});
  });
 });
 E.AddFreeform(B,TEXT("Slept through: the morning after at SARO, the walkie"),TEXT("as if slept through tonight; 09:00 the next day in the control room: the array found turned, a hail, answered"),[this,Haps,Shot,Report]
 {
  FS47Happenings* H=Haps();FString Why;
  if(!H||!H->DevBigSlept(FString(),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  FStage Stage;Stage.From=9;Stage.To=12;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps,Shot,Report]
  {
   FS47Happenings* Hg=Haps();if(Hg)Hg->DevBigMorning();
   WaitFor([this]{return M->FieldRadio.IsValid()&&M->FieldRadio->Hailing();},20,[this,Haps,Shot,Report]
   {
    Shot(TEXT("morning-hail"));
    After(2,[this]{if(M->FieldRadio.IsValid())M->FieldRadio->PressTalk();});
    After(6,[Shot]{Shot(TEXT("morning-walkie-1"));});
    After(13.5,[Shot]{Shot(TEXT("morning-walkie-2"));});
    After(24,[this,Haps,Report]{FS47Happenings* Hr=Haps();Tell(Hr&&Hr->BigHas(TEXT("dale"))&&Hr->BigHas(TEXT("array")),Report(TEXT("the morning after")),6);});
   },[this,Report]{Tell(false,TEXT("no hail came on the walkie: ")+Report(TEXT("the morning after")));});
  });
 });
 E.AddFreeform(B,TEXT("The morning after: the array from the yard"),TEXT("the yard by day; every dish on remote where it went in the night"),[this,Shot,Report,Left,ArrayMiddle]
 {
  FStage Stage;Stage.From=9;Stage.To=17;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Shot,Report,Left,ArrayMiddle]{AimAt(ArrayMiddle());After(2,[Shot]{Shot(TEXT("morning-array"));});After(3,[this,Report,Left]{Tell(Left({TEXT("array"),TEXT("turned")}),Report(TEXT("the array by day")),6);});});
 });
 E.AddFreeform(B,TEXT("The evening after: the CB"),TEXT("20:30, in the truck on the highway (the CB is under its dash); somebody who was out on the road that night"),[this,Haps,Report,Road]
 {
  Road(20.5,22.5,false,[this,Haps,Report]
  {
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return Hw&&Hw->BigHas(TEXT("cb"));},30,[this,Report]{Tell(true,Report(TEXT("on the CB")),6);},[this,Report]{Tell(false,TEXT("nothing on the CB: ")+Report(TEXT("on the CB")));});
  });
 });
 E.AddFreeform(B,TEXT("The night after: 1420 in the truck"),TEXT("00:30 the night after, the truck on the highway, the radio on 1420 (Earl's Night Line, on the air from eight in the evening to four); a caller about the lights, in the program's own turn"),[this,Road,Haps,Report]
 {
  Road(.5,3.5,false,[this,Haps,Report]
  {
   if(M->CarRadio.IsValid())M->CarRadio->Set(true,TEXT("caliche"));
   WaitFor([Haps]{FS47Happenings* Hw=Haps();return Hw&&Hw->BigHas(TEXT("radio"));},55,[this,Report]{Tell(true,Report(TEXT("on 1420")),6);},[this,Report]{Tell(false,TEXT("the program never gave the caller a turn: ")+Report(TEXT("on 1420")));});
  });
 });
 for(const TCHAR* Shape:{TEXT("column"),TEXT("fall"),TEXT("rise")})
 {
  const FString V=Shape;
  E.AddFreeform(B,FString::Printf(TEXT("Go: the marked ground (%s)"),Shape),TEXT("by day, on foot beside it; E on it: the panel, the record and the lead"),[this,Haps,Shot,Report,Left,V]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevBigSlept(V,Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   FStage Stage;Stage.From=10;Stage.To=17;Stage.Area=TEXT("road");Stage.Yaw=0;
   Stage.Where=[Haps]{FVector At;FS47Happenings* Hs=Haps();if(Hs&&Hs->SiteHere(TEXT("bigmark"),At))return FVector2D(At.X+3,At.Z+7);return FVector2D(8000,0);};
   Go(Stage,[this,Haps,Shot,Report,Left,V]
   {
    FS47Happenings* Hn=Haps();FVector At;
    if(!Hn||!Hn->SiteHere(TEXT("bigmark"),At)){Tell(false,TEXT("the mark is not in this area"));return;}
    AimAt(At);
    After(2.5,[Shot,V]{Shot(V+TEXT("-mark"));});
    // The panel holds the menu's timers while it is open: its picture and the line's word go with opening it.
    After(4,[this,Haps,Shot,Report,Left,V]{FS47Happenings* Hu=Haps();if(Hu)Hu->DevUseBig();Shot(V+TEXT("-mark-panel"));Tell(Left({TEXT("trace")}),Report(V+TEXT(" mark")),6);});
   });
  });
 }
}
