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
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

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
  E.AddFreeform(G,Label,TEXT("21:00, control room, on the base set; where he is goes on the map"),[this,Haps,V]
  {
   FStage Stage;Stage.From=21;Stage.To=24;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Haps,V]
   {
    FS47Happenings* H=Haps();FString Why;
    if(!H||!H->DevStart(TEXT("cb_stranded"),V,Why)){Tell(false,Why);return;}
    Tell(H->HasSite(TEXT("truck")),FString::Printf(TEXT("The call comes on the base set. Where he says he is, is on your map (TAB)%s."),V==TEXT("wrong")?TEXT("; a later night the voice may come back"):TEXT("")),7);
   });
  });
 }
 E.AddFreeform(G,TEXT("Go: the stuck driver's rig"),TEXT("on foot beside it, at night; E on it. A new real call first, so there is someone to help"),[this,Haps,ToSite]
 {
  FS47Happenings* H=Haps();FString Why;
  if(!H||!H->DevStart(TEXT("cb_stranded"),TEXT("real"),Why)){Tell(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  ToSite(TEXT("truck"),10,TEXT("His rig, ahead of you, and him beside it. E on it."));
 });
 E.AddFreeform(G,TEXT("CB: the voice says what you did"),TEXT("02:30, control room; a danger, after a voice that was not right"),[this,Haps]
 {
  FStage Stage;Stage.From=2.5;Stage.To=4.5;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Haps]
  {
   FS47Happenings* H=Haps();FString Why;
   if(!H||!H->DevStart(TEXT("cb_echo"),FString(),Why)||!H->Live(TEXT("cb_echo"))){Tell(false,Why);return;}
   Tell(true,TEXT("The base set opens by itself, and the voice says something you did."),6);
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
   Tell(true,TEXT("Lights coming down far out. Keep looking: in twenty seconds they are on the ground and the bearing is on your map."),7);
   After(28,[this,Haps]
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
 E.AddFreeform(G,TEXT("Humour: a pink convertible at the diner"),TEXT("21:30, the diner's lot; engine running, nobody in it; E on it"),[this,Humor]
 {
  FStage Stage;Stage.From=21.5;Stage.To=1;Stage.Area=TEXT("diner");Stage.Yaw=0;
  Stage.Where=[this]{return S47HapDev::CatalogSpot(M,TEXT("diner"),TEXT("diner"),TEXT("diner"),FVector2D(7978,1972),-10);};
  Go(Stage,[Humor]{Humor(TEXT("car"),TEXT("A pink convertible, running, nobody in it. E on it."));});
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
 E.AddFreeform(G,TEXT("Humour: a small man at the pump wants water"),TEXT("22:00, the pump in Caliche; E talks to him"),[this,Humor]
 {
  FStage Stage;Stage.From=22;Stage.To=1;Stage.Area=TEXT("diner");Stage.Yaw=0;
  Stage.Where=[this]
  {
   auto* F=Session();
   const FVector Gas=F&&F->Director?F->Director->LightPlacePoint(TEXT("gas")):FVector::ZeroVector;
   return S47HapDev::CatalogSpot(M,TEXT("__none"),TEXT("gas"),TEXT("diner"),Gas.IsZero()?FVector2D(7995,2250):FVector2D(Gas.X,Gas.Z+8),8);
  };
  Go(Stage,[Humor]{Humor(TEXT("pump"),TEXT("A small grey man in a shirt and a hat, at the pump. E talks to him."));});
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
  {TEXT("room6_tv"),TEXT("16. Room 6: the television on, nobody"),TEXT("23:00, the motel lot"),5,nullptr},
  {TEXT("town_blackout"),TEXT("17. Caliche goes dark for a minute"),TEXT("22:00, Caliche's street"),1,nullptr},
  {TEXT("array_burst"),TEXT("18. The array catches a burst: a bearing"),TEXT("23:00, control room; the bearing goes on the map"),2,nullptr},
  {TEXT("am_caller"),TEXT("19. 1420: a caller tells last night wrong"),TEXT("21:00, in the truck, when it is the caller's turn on the programme"),4,nullptr},
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
  E.AddFreeform(R,Line.Label,Line.Note,[this,Haps,Road,ToSite,Id,St,Site]
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
   if(St==4){Road(Id==TEXT("car_light")?2.5:21,4.5,false,Fire);return;}
   if(St==0){ToLandPlace(TEXT("windmill"),40,Id==TEXT("dust_devil")?21:23,4.5,[Fire](double,double){Fire();});return;}
   FStage Stage;Stage.To=4.5;Stage.Yaw=UE_DOUBLE_PI;
   if(St==1){Stage.From=Id==TEXT("phone_empty")?23:22;Stage.Area=TEXT("diner");Stage.Yaw=0;Stage.Where=[]{return FVector2D(7995,2110);};}
   else if(St==2){Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::ControlRoom();};}
   else if(St==3){Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{return S47HapDev::Yard();};}
   else{Stage.From=23;Stage.Area=TEXT("saro");Stage.Where=[]{const FS47Box2& Park=S47Legs::MOTEL_PARK();return FVector2D(Park.MinX-6,Park.MinZ+6);};}
   Go(Stage,Fire);
  });
 }
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
}
