#include "S47DevFreeform.h"
#include "S47Aftermath.h"
#include "S47CalicheAccess.h"
#include "S47CaseFieldTests.h"
#include "S47DevEvents.h"
#include "S47DoorVisit.h"
#include "S47EventPlaces.h"
#include "S47OpenLand.h"
#include "S47FarmHouse.h"
#include "S47FarmSite.h"
#include "S47FieldRadio.h"
#include "S47FloodProbe.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Glimpses.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Rhythm.h"
#include "S47Night.h"
#include "S47Openings.h"
#include "S47Places.h"
#include "S47Predecessor.h"
#include "S47PredecessorText.h"
#include "S47RadioTips.h"
#include "S47RoadTerrain.h"
#include "S47RoomLights.h"
#include "S47SkyVisitors.h"
#include "S47SmallHints.h"
#include "S47Taken.h"
#include "S47Theme.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Legs.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DevFreeform,Log,All);

namespace
{
 // Hours as the night reckons them: 19 to 30 is seven in the evening to six in the morning.
 const TCHAR* const NoDevRun=TEXT("this needs a developer run, where nothing is saved. F10, Run the night, Go to a test run.");
 // The small hours: past the latest any night's bodies start (FS47NightRhythm::LatestHour), before six.
 constexpr double SmallFrom=3.2,SmallTo=5.5;
 constexpr double EveningFrom=21,EveningTo=24;
 FVector2D ControlRoom(){return FVector2D(0,-1.2);}
 FVector2D Yard(){return FVector2D(7,19);}
 const TCHAR* const PlaceIds[]={TEXT("cinema"),TEXT("feed"),TEXT("church"),TEXT("gas")};
 const TCHAR* const PlaceNames[]={TEXT("the Plaza cinema"),TEXT("the feed store"),TEXT("the church"),TEXT("the gas station")};
 const TCHAR* const TakerNames[3]={TEXT("small Roswell"),TEXT("crab"),TEXT("tall one")};
 const TCHAR* const SpeedLabels[]={TEXT("x1"),TEXT("x5"),TEXT("x15"),TEXT("x30"),TEXT("x60")};
 const double SpeedValues[]={1,5,15,30,60};
 FString ClockText(double Clock)
 {
  const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));
  return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);
 }
 // What the screen shows while time runs: small, top right, and in the way of nothing.
 class SS47DevPanel:public SCompoundWidget
 {
 public:
  SLATE_BEGIN_ARGS(SS47DevPanel){}
   SLATE_ARGUMENT(FS47DevFreeform*,Owner)
  SLATE_END_ARGS()
  void Construct(const FArguments& InArgs)
  {
   S47_USING_THEME
   Owner=InArgs._Owner;
   SetVisibility(EVisibility::HitTestInvisible);
   ChildSlot.HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(Pad(0,16,16,0))
   [
    SNew(SBorder).BorderImage(Box(Night(.72f))).Padding(Pad(14,10,14,10))
    [
     SNew(STextBlock).Font(Term(20)).ColorAndOpacity(Amber(.95f))
     .Text_Lambda([this]
     {
      const double Now=FPlatformTime::Seconds();
      // Not every frame: it reads a good part of the game.
      if(Owner&&Now-LastRead>.5){LastRead=Now;Cached=FText::FromString(Owner->PanelText());}
      return Cached;
     })
    ]
   ];
  }
 private:
  FS47DevFreeform* Owner=nullptr;
  mutable double LastRead=0;
  mutable FText Cached;
 };
}

FS47DevFreeform::~FS47DevFreeform()
{
 if(Panel&&!IsEngineExitRequested()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
 Panel.Reset();
}

FS47FreeformSession* FS47DevFreeform::Session() const{return M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;}
bool FS47DevFreeform::DevRun() const{return M&&M->Flow&&M->Flow->bDevRun;}

void FS47DevFreeform::Tell(bool bStarted,const FString& Text,float Seconds)
{
 bLastOk=bStarted;LastText=Text;
 if(M&&M->UI)M->UI->Toast((bStarted?FString():FString(TEXT("Not started: ")))+Text,Seconds);
 UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 %s: %s"),bStarted?TEXT("started"):TEXT("refused"),*Text);
}

void FS47DevFreeform::After(double Seconds,TFunction<void()> Fn){FWait W;W.Left=Seconds;W.Fn=MoveTemp(Fn);Waiting.Add(MoveTemp(W));}
void FS47DevFreeform::WaitFor(TFunction<bool()> Cond,double Timeout,TFunction<void()> Then,TFunction<void()> Else)
{FWait W;W.Left=Timeout;W.Cond=MoveTemp(Cond);W.Fn=MoveTemp(Then);W.Else=MoveTemp(Else);Waiting.Add(MoveTemp(W));}

void FS47DevFreeform::ToHour(double From,double To)
{
 auto* F=Session();if(!F||From<0)return;
 const double Hour=FMath::Fmod(F->Clock(),86400.)/3600.;
 const bool bInside=To<From?(Hour>=From||Hour<To):(Hour>=From&&Hour<To);
 if(bInside)return;
 F->TestAdvance(FMath::Fmod(From*3600.-FMath::Fmod(F->Clock(),86400.)+86400.,86400.));
}

void FS47DevFreeform::AimAt(const FVector& At)
{
 if(!M||!M->Player)return;
 const FVector D=At-FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
 M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
}

void FS47DevFreeform::ClearRunning()
{
 auto* F=Session();if(!F)return;
 Waiting.Reset();
 if(M->World){M->World->TestInput=nullptr;M->World->Autopilot.Reset();}
 if(F->Director&&DevRun()&&F->Director->Busy())F->Director->CancelForTest();
 if(F->IndoorDread)F->IndoorDread->DevCancel();
 if(F->Taken&&F->Taken->Busy()){F->Taken->Stop();if(M->UI)M->UI->Fade(false);} // Reached by a crab goes dark at once: Stop leaves that to whoever asked.
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())
 {
  // Over for this site, as a Continue would find it: the marks as they stand.
  const auto Saved=F->FarmSite->Save();F->FarmSite->Stop();F->FarmSite->Begin(Saved);
 }
 // Its quiet time after it counts as busy for everything else: a cancelled one is gone altogether in a test run.
 if(M->HighwayUfo&&M->HighwayUfo->Busy()){M->HighwayUfo->CancelForTest();if(DevRun())M->HighwayUfo->Begin(nullptr);}
 if(M->SkyVisitors)M->SkyVisitors->DevFormationStop();
 if(M->Night&&M->Night->Glimpses)M->Night->Glimpses->DevClear();
 S47FloodProbe::StopLive(M);
}

void FS47DevFreeform::Go(const FStage& Stage,TFunction<void()> Then)
{
 auto* F=Session();
 if(!F||!M->World||!M->Web||!M->Player){Tell(false,TEXT("Freeform only"));return;}
 ClearRunning();
 if(M->World->bDriving)M->World->StopDriving();
 if(Stage.Day>0&&F->Day()<Stage.Day)F->TestAdvance((Stage.Day-F->Day())*86400.);
 ToHour(Stage.From,Stage.To);
 const auto Stand=[this,Stage,Then]
 {
  // A moment for the scene and the floor under the new place. The next line of the menu's own work sees it standing there.
  if(Stage.Where&&M->Player){const FVector2D At=Stage.Where();M->Player->Place(At.X,At.Y,Stage.Yaw);M->Player->Pitch=Stage.Pitch;}
  After(.35,Then);
 };
 if(!Stage.Area.IsEmpty()&&M->Web->Area()!=Stage.Area)
 {
  if(Stage.Area==TEXT("diner"))M->World->PlaceTruck(TEXT("diner"));
  M->World->Enter(Stage.Area);
  After(.7,Stand);
 }
 else Stand();
}

FString FS47DevFreeform::WhyRefused(const FString& Id) const
{
 auto* F=Session();
 if(!F||!F->Director)return TEXT("Freeform only");
 if(!DevRun())return NoDevRun;
 auto& D=*F->Director;
 if(D.Busy())return FString::Printf(TEXT("%s is on"),*D.ActiveEvent());
 if(F->Held())return TEXT("the game is held: a panel, a menu, or a trip being set up");
 if(M->HighwayUfo&&M->HighwayUfo->Busy())return TEXT("the highway encounter or its rest is on; Stop everything clears it");
 if(F->Taken&&F->Taken->Busy())return TEXT("being taken");
 const bool bBody=Id==TEXT("observer")||Id==TEXT("service_presence")||Id==TEXT("feeder_presence")||Id==TEXT("array_presence");
 const auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();
 if(bBody&&(!G||!G->HasNativeBody()))return TEXT("the Roswell body is not in this build");
 if(bBody&&!G->Individuals().IsEmpty())return TEXT("other bodies are out");
 if((Id==TEXT("array_presence")||Id==TEXT("s03_failure"))&&F->Field&&F->Field->HasArrayFault())return TEXT("the array already has a fault: repair it first");
 if((Id==TEXT("s03_failure")||Id==TEXT("feeder_fault"))&&F->PowerFault())return TEXT("the power is already out: repair the feeder first");
 if(Id==TEXT("empty_car"))return TEXT("the car is in view, or missing from this scene");
 if(Id==TEXT("landing_trace"))return TEXT("the marked ground is in view, or there is no open ground within reach on this ground");
 if(Id==TEXT("roof_steps"))return TEXT("a roof test has the window, or that case is closed");
 if(Id==TEXT("roomlight_return"))return TEXT("the switch has to be off, you gone 60 s, over 25 m away and the room out of sight");
 return FString::Printf(TEXT("the Director refused in %s"),*M->Web->Area());
}

void FS47DevFreeform::StartDirector(const FString& Id,const FStage& Stage,bool bAim,const FString& Say,TFunction<void()> Before)
{
 if(!Session()){Tell(false,TEXT("Freeform only"));return;}
 Go(Stage,[this,Id,bAim,Say,Before]
 {
  auto* F=Session();if(!F||!F->Director){Tell(false,TEXT("Freeform only"));return;}
  if(Before)Before();
  if(!F->Director->Force(Id)){Tell(false,WhyRefused(Id));return;}
  if(bAim)AimAt(F->Director->EventAnchor());
  Tell(true,Say);
 });
}

// ---- Directors's catalogue ----
void FS47DevFreeform::AddDirector(FS47DevEvents& E)
{
 struct FLine{const TCHAR* Group;const TCHAR* Id;const TCHAR* Label;const TCHAR* Note;const TCHAR* Say;double From,To;int32 Spot;bool bAim;};
 enum{SControl,SYard,SQuiet};
 static const TCHAR* const Tools=TEXT("Director: instruments");
 static const TCHAR* const Faults=TEXT("Director: faults and marks");
 static const TCHAR* const Small=TEXT("Director: small things");
 static const TCHAR* const Far=TEXT("Director: far off");
 static const TCHAR* const Figures=TEXT("Director: figures");
 static const FLine Lines[]={
  {Tools,TEXT("extra_pulse"),TEXT("Extra pulse"),TEXT("11:00, control room; shows on the next reading"),TEXT("Extra pulse armed. Take a reading at OPS: it carries extra ticks. Compare it at the receiver."),11,18,SControl,false},
  {Tools,TEXT("wrong_source"),TEXT("Wrong source"),TEXT("11:00, control room; shows on the next reading"),TEXT("Wrong source armed. The next reading's extra component does not follow the sky source."),11,18,SControl,false},
  {Tools,TEXT("time_disagreement"),TEXT("Time disagreement"),TEXT("11:00, control room; shows on the next reading"),TEXT("Time disagreement armed. Two records of the next reading will not agree."),11,18,SControl,false},
  {Tools,TEXT("duplicate_print"),TEXT("Duplicate print"),TEXT("11:00, control room; shows on the next reading"),TEXT("Duplicate print armed. The printer gives one more than you asked for on the next reading."),11,18,SControl,false},
  {Tools,TEXT("unsigned_telex"),TEXT("Unsigned telex"),TEXT("11:00, control room; the printer runs"),TEXT("The printer runs: an unsigned telex at the machine."),11,18,SControl,false},
  {Faults,TEXT("s03_failure"),TEXT("Antenna S-03 fails"),TEXT("11:00, control room; repair it to end it"),TEXT("S-03 is down. It is on ARRAY STATUS; repairing it ends this."),11,18,SControl,false},
  {Faults,TEXT("feeder_fault"),TEXT("Feeder fault"),TEXT("11:00, control room; the power goes"),TEXT("The feeder is down and the power with it. Repair it at the road cabinet to end this."),11,18,SControl,false},
  {Faults,TEXT("landing_trace"),TEXT("Marked ground and fragment"),TEXT("11:00, control room; the place is out in the east yard"),TEXT("Marked ground and a fragment near the east yard, about x 12, z -12: out the door and to your right."),11,18,SControl,false},
  {Small,TEXT("roof_steps"),TEXT("Steps on the roof"),TEXT("22:00, control room"),TEXT("Steps overhead for 45 seconds. Go out to the service yard and they count as checked."),EveningFrom,EveningTo,SControl,false},
  {Small,TEXT("empty_car"),TEXT("Empty car"),TEXT("22:00, control room; it stands in the staff bay"),TEXT("An empty car in the staff bay for three minutes, east of the door."),EveningFrom,EveningTo,SControl,false},
  {Small,TEXT("flyover"),TEXT("Flyover"),TEXT("22:00, yard, looking at it"),TEXT("A light crosses overhead, with a low thud after it."),EveningFrom,EveningTo,SYard,true},
  {Far,TEXT("response_lights"),TEXT("Distant lights"),TEXT("22:00, yard, looking at them"),TEXT("Lights far out for 100 seconds. A controlled test at the receiver is what makes them a finding."),EveningFrom,EveningTo,SYard,true},
  {Far,TEXT("quiet_zone"),TEXT("Quiet zone"),TEXT("22:00, yard; walk north a few metres"),TEXT("A patch where the room tone stops, at x 12, z -17. Walk in and out of it for 150 seconds."),EveningFrom,EveningTo,SQuiet,false},
  {Figures,TEXT("observer"),TEXT("Figure at the antennas"),TEXT("03:12, yard"),TEXT("Something shifts along the fence: a figure comes to the antennas in about six seconds."),SmallFrom,SmallTo,SYard,true},
  {Figures,TEXT("service_presence"),TEXT("Service yard: scrape, then it hunts"),TEXT("03:12, yard; catches you if it can"),TEXT("A scrape at the service door. In six seconds it hunts you: get indoors and shut the door. Contact ends the encounter."),SmallFrom,SmallTo,SYard,true},
  {Figures,TEXT("close_pass"),TEXT("Close pass: a low craft"),TEXT("03:12, yard, looking at where it comes from"),TEXT("A low craft comes past within a minute. Keep looking this way."),SmallFrom,SmallTo,SYard,true},
 };
 const auto Spot=[](int32 S){return S==SControl?ControlRoom():S==SQuiet?FVector2D(12,-10):Yard();};
 for(const FLine& L:Lines)
 {
  FStage Stage;Stage.From=L.From;Stage.To=L.To;Stage.Area=TEXT("saro");const int32 S=L.Spot;Stage.Where=[Spot,S]{return Spot(S);};Stage.Yaw=UE_DOUBLE_PI;
  const FString Id=L.Id,Say=L.Say;const bool bAim=L.bAim;
  E.AddFreeform(L.Group,L.Label,L.Note,[this,Id,Stage,bAim,Say]{StartDirector(Id,Stage,bAim,Say);});
 }
 // Where the Director's own rule puts them: at the cabinet they belong to.
 for(const bool bFeeder:{true,false})
 {
  const FString Id=bFeeder?TEXT("feeder_presence"):TEXT("array_presence"),Point=bFeeder?TEXT("ff:feeder"):TEXT("ff:service");
  E.AddFreeform(Figures,bFeeder?TEXT("Figure at the road feeder"):TEXT("Figure at the service cabinet"),bFeeder?TEXT("03:12, at the feeder on the road"):TEXT("03:12, at the cabinet; it can cause a fault there"),[this,Id,Point,bFeeder]
  {
   FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Area=bFeeder?TEXT("road"):TEXT("saro");
   Stage.Where=[this,Point]{auto* F=Session();const FVector P=F&&F->Field?F->Field->InteractionPoint(Point):FVector::ZeroVector;return FVector2D(P.X,P.Z+4.);};
   StartDirector(Id,Stage,true,bFeeder?TEXT("A figure comes to the feeder in six seconds. Stay within 12 m or it does not."):TEXT("A figure comes to the cabinet in six seconds. Stay within 12 m or it does not."));
  });
 }
 // The small things: where each can be noticed, at 22:00, and with the screen saying so, since some are easy to miss.
 for(const FString& Id:S47SmallHints::Ids())
 {
  E.AddFreeform(Small,S47SmallHints::TitleOf(Id),S47SmallHints::NoteOf(Id),[this,Id]
  {
   FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");Stage.Yaw=UE_DOUBLE_PI;
   const bool bOut=Id==TEXT("antenna_creak")||Id==TEXT("fence_tap")||Id==TEXT("night_hush");
   const bool bFar=Id==TEXT("service_door_open");
   Stage.Where=[bOut,bFar]{return bFar?FVector2D(7,48):bOut?Yard():ControlRoom();};
   StartDirector(Id,Stage,false,S47SmallHints::NoteOf(Id));
  });
 }
 // The car with the light over it.
 E.AddFreeform(Small,TEXT("Empty car: light over it, can take you"),TEXT("03:12, yard, the car behind you within 32 m"),[this]
 {
  FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(27,24);};Stage.Yaw=UE_DOUBLE_PI;
  StartDirector(TEXT("empty_car"),Stage,false,TEXT("Turn round: a light stands over the empty car, and it can come for you. Distance or a wall gets you out of it."),[this]{if(auto* F=Session())F->Director->TestCarDice(0.f);});
  After(4.,[this]{if(auto* F=Session())F->Director->TestCarDice(bNatural?FMath::FRand():-1.f);});
 });
 // The room light has no shortcut: its own rule asks for a real switch, a real departure and a real minute.
 E.AddFreeform(Small,TEXT("Room light comes back on"),TEXT("switches it off, takes you out, 60 seconds, then it is on"),[this]{StartRoomLight();});
 // ---- lights over a place ----
 static const TCHAR* const Lights=TEXT("Lights over a place");
 E.AddFreeform(Lights,TEXT("Over the Alderete place, from the yard"),TEXT("22:00, yard, looking south; leaves marks when it ends"),[this]
 {
  FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
  StartDirector(TEXT("farm_lights"),Stage,true,TEXT("A caller reports lights over the Alderete place. They go when you come within 130 m, and leave marks."));
 });
 for(int32 I=0;I<4;++I)
 {
  const FString Place=PlaceIds[I],Name=PlaceNames[I];
  E.AddFreeform(Lights,FString::Printf(TEXT("Over %s, from the street"),*Name),TEXT("22:00, Caliche, looking at them; leaves marks when it ends"),[this,Place,Name]
  {
   FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("diner");Stage.Where=[]{return FVector2D(7995,2250);};
   Go(Stage,[this,Place,Name]
   {
    auto* F=Session();if(!F||!F->Director)return;
    if(!F->Director->ForceLights(Place)){Tell(false,F->Director->LightPlacePoint(Place).IsZero()?FString::Printf(TEXT("%s is not in this scene"),*Name):WhyRefused(TEXT("town_lights")));return;}
    AimAt(F->Director->EventAnchor());
    Tell(true,FString::Printf(TEXT("Somebody reports lights low over %s. They go when you come within 70 m, and leave marks."),*Name));
   });
  });
  E.AddFreeform(Lights,FString::Printf(TEXT("Over %s, as heard at SARO"),*Name),TEXT("22:00, yard; a radio tip, a drive away"),[this,Place,Name]
  {
   FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Place,Name]
   {
    auto* F=Session();if(!F||!F->Director)return;
    if(!F->Director->ForceLights(Place)){Tell(false,WhyRefused(TEXT("town_lights")));return;}
    Tell(true,FString::Printf(TEXT("The tip for %s goes out now. They hold for the drive there, about three minutes more than the usual."),*Name));
   });
  });
 }
 // ---- the low lights ----
 static const TCHAR* const Orbs=TEXT("Low lights (orbs)");
 const auto Orb=[this](double From,double To,const TCHAR* Say)
 {
  FStage Stage;Stage.From=From;Stage.To=To;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(FS47DuskOrb::HomeX,FS47DuskOrb::HomeZ+18);};Stage.Yaw=0;
  StartDirector(TEXT("dusk_orb"),Stage,true,Say);
 };
 E.AddFreeform(Orbs,TEXT("At dusk among the antennas"),TEXT("18:40, 18 m from S-02 and S-03, looking at them"),[Orb]{Orb(18.66,19.25,TEXT("One to three low lights among the antennas for up to five minutes. They flee at 7 m. The camera keeps a record."));});
 E.AddFreeform(Orbs,TEXT("In the dark among the antennas"),TEXT("21:00, the same event at night"),[Orb]{Orb(21,23.2,TEXT("The low lights in the dark. They flee at 7 m."));});
}

void FS47DevFreeform::StartRoomLight()
{
 auto* F=Session();
 if(!F||!M->RoomLights||!M->Player||!M->Inter){Tell(false,TEXT("Freeform only"));return;}
 // A light needs its supply: a fault left by an earlier line would make it a minute's wait for nothing.
 if(F->PowerFault()){Tell(false,TEXT("the power is out. Time and place, Mend the power, then try again"));return;}
 FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");
 Stage.Where=[this]{const FVector P=M->RoomLights->ApproachPoint(TEXT("control"));return FVector2D(P.X,P.Z);};
 Go(Stage,[this]
 {
  if(!M->RoomLights)return;
  AimAt(M->RoomLights->InteractionPoint(TEXT("control")));
  // Its next frame sees the camera on the switch.
  After(.25,[this]
  {
   auto& L=*M->RoomLights;
   if(!L.ManualEnabled(TEXT("control"))||L.Use(TEXT("roomlight:control")))
   {
    // Down the yard, with the room behind you: this is the rule's own requirement.
    M->Player->Place(7,48,UE_DOUBLE_PI);M->Player->Pitch=0;
    Tell(true,TEXT("Control room switch off, and you are out of there. In 60 seconds the light is on again: then walk back and look."),8);
    After(61.,[this]
    {
     auto* S=Session();if(!S||!S->Director)return;
     if(!S->Director->Force(TEXT("roomlight_return"))){Tell(false,WhyRefused(TEXT("roomlight_return")));return;}
     Tell(true,TEXT("The control room light is on, with its switch still off. Go and look at the switch."));
    });
   }
   else Tell(false,TEXT("the switch could not be reached from its own spot"));
  });
 });
}

// ---- indoors ----
void FS47DevFreeform::PlaceRoom(const FString& Place,TFunction<void()> Then)
{
 FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;
 if(Place==TEXT("house"))
 {
  const FVector Room=S47FarmHouse::Point(6,4.4)+FVector(-24,0,300);
  Stage.Area=TEXT("saro");Stage.Where=[Room]{return FVector2D(Room.X,Room.Z);};
 }
 else if(Place==TEXT("saro")){Stage.Area=TEXT("saro");Stage.Where=[]{return ControlRoom();};}
 else
 {
  FVector Room;if(!S47CalicheAccess::RoomMiddle(Place,Room)){Tell(false,FString::Printf(TEXT("no room %s in this scene"),*Place));return;}
  if(S47CalicheAccess::IsOpen(Place))S47CalicheAccess::UseDoor(Place);
  Stage.Area=TEXT("diner");Stage.Where=[Room]{return FVector2D(Room.X,Room.Z);};
  // Facing its own street door: the glass and the frame the room has are that way.
  FVector Door,Normal;
  if(S47CalicheAccess::DoorAt(Place,Door,Normal))Stage.Yaw=FMath::Atan2(-(Door.X-Room.X),-(Door.Z-Room.Z));
 }
 Go(Stage,MoveTemp(Then));
}

void FS47DevFreeform::StartDoor(const FString& Place,bool bLight)
{
 PlaceRoom(Place,[this,bLight]
 {
  auto* F=Session();if(!F||!F->IndoorDread||!M->Player||!M->Web)return;
  // The light comes through a door only on a night that has already lit a room (FS47DoorVisit::BeginOpening).
  if(bLight)F->IndoorDread->SurvivedFlood(FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z),M->Web->Area(),0,TEXT("this room"));
  const FString Why=F->IndoorDread->Door().DevStart();
  if(!Why.IsEmpty()){Tell(false,Why);return;}
  AimAt(F->IndoorDread->Door().DoorMiddle()+FVector(0,1.1,0));
  Tell(true,bLight?TEXT("The handle moves. Leave it and the door opens on the light, with the way out it always has."):TEXT("The handle moves. E on the door holds it. While it waits, E opens it. Do nothing and it comes open by itself."),8);
 });
}

void FS47DevFreeform::StartPeek(const FString& Place)
{
 PlaceRoom(Place,[this]
 {
  auto* F=Session();if(!F||!F->IndoorDread)return;
  auto& Dread=*F->IndoorDread;
  if(!Dread.DevPeek()){Tell(false,TEXT("not in a place it looks into, or something else is on"));return;}
  Tell(true,TEXT("Looking for a window or door edge you can see from here."),4);
  WaitFor([&Dread]{return Dread.PeekActive();},14,[this]{Tell(true,TEXT("Something is at the window. It ducks when you look."));},[this]{Tell(false,TEXT("no window or door edge in sight from here"));});
 });
}

void FS47DevFreeform::AddIndoors(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Indoors: door, window, lit room");
 struct FRoom{const TCHAR* Id;const TCHAR* Name;};
 static const FRoom Rooms[]={{TEXT("house"),TEXT("the old house")},{TEXT("cinema"),TEXT("the Plaza cinema")},{TEXT("feed"),TEXT("the feed store")},{TEXT("church"),TEXT("the church")}};
 for(const FRoom& R:Rooms)
 {
  const FString Id=R.Id;
  E.AddFreeform(G,FString::Printf(TEXT("Door: something tries it, %s"),R.Name),TEXT("03:12, in the room"),[this,Id]{StartDoor(Id,false);});
 }
 E.AddFreeform(G,TEXT("Door: it opens on the light, the old house"),TEXT("03:12; the way a door visit can end in being taken"),[this]{StartDoor(TEXT("house"),true);});
 E.AddFreeform(G,TEXT("Door: try it where you stand"),TEXT("as you are, past the hour and the dice"),[this]
 {
  auto* F=Session();if(!F||!F->IndoorDread){Tell(false,TEXT("Freeform only"));return;}
  const FString Why=F->IndoorDread->Door().DevStart();
  if(!Why.IsEmpty())Tell(false,Why);else Tell(true,TEXT("The handle moves."));
 });
 static const FRoom Peeks[]={{TEXT("saro"),TEXT("SARO control room")},{TEXT("house"),TEXT("the old house")},{TEXT("cinema"),TEXT("the Plaza cinema")}};
 for(const FRoom& R:Peeks)
 {
  const FString Id=R.Id;
  E.AddFreeform(G,FString::Printf(TEXT("Window: a head rises, %s"),R.Name),TEXT("03:12, in the room"),[this,Id]{StartPeek(Id);});
 }
 for(int32 Who=0;Who<3;++Who)
  E.AddFreeform(G,FString::Printf(TEXT("Lit room comes back: %s, the old house"),TakerNames[Who]),TEXT("03:12; a room once escaped from lights up again"),[this,Who]
  {
   PlaceRoom(TEXT("house"),[this,Who]
   {
    auto* F=Session();if(!F||!F->IndoorDread||!F->Taken||!M->Web)return;
    F->IndoorDread->SurvivedFlood(FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z),M->Web->Area(),Who,TEXT("the old house"));
    if(!F->Taken->Begin(ES47Taker(Who),TEXT("the old house"))){Tell(false,TEXT("being taken already, or no way to begin here"));return;}
    Tell(true,TEXT("Steps overhead, then the light at the window. Get out of it: a wall, or distance."));
   });
  });
}

// ---- marks and visits ----
void FS47DevFreeform::AddSites(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Marks and visits");
 // In their hours: at the marks, 12 seconds until the night's own rule would send one.
 const auto ToMarks=[this](const FString& Place,TFunction<void()> Then)
 {
  auto* F=Session();if(!F||!F->FarmSite){Tell(false,TEXT("Freeform only"));return;}
  FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;
  Stage.Area=Place==TEXT("farm")?TEXT("saro"):TEXT("diner");
  if(Place==TEXT("farm"))Stage.Where=[]{return FVector2D(84+6,492+6);};else Stage.Where=[]{return FVector2D(7995,2250);};
  Go(Stage,[this,Place,Then]
  {
   auto* S=Session();if(!S||!S->FarmSite||!S->Director)return;
   if(Place==TEXT("farm"))S->FarmSite->ArmFarm(S->Clock());
   else
   {
    const FVector Point=S->Director->LightPlacePoint(Place);
    if(Point.IsZero()){Tell(false,FString::Printf(TEXT("%s is not in this scene"),*Place));return;}
    S->FarmSite->Arm(S->Clock(),Place,S->Director->LightPlaceTitle(Place),M->Web->Area(),Point);
   }
   const FVector C=S->FarmSite->Centre();
   M->Player->Place(C.X+6,C.Z+6,0);M->Player->Pitch=0;AimAt(C);
   After(.2,Then);
  });
 };
 TArray<TPair<FString,FString>> Places;
 Places.Add({TEXT("farm"),TEXT("the Alderete place")});
 for(int32 I=0;I<4;++I)Places.Add({PlaceIds[I],PlaceNames[I]});
 for(const auto& P:Places)
 {
  const FString Id=P.Key,Name=P.Value;
  E.AddFreeform(G,FString::Printf(TEXT("Marks at %s"),*Name),TEXT("03:12, takes you to them; nothing comes"),[this,ToMarks,Id,Name]
  {
   ToMarks(Id,[this,Name]{auto* F=Session();if(F&&F->FarmSite)F->FarmSite->TestRollWho(-1.f);Tell(true,FString::Printf(TEXT("Nine marks, a low haze and an occasional sound at %s. Until noon. E on them keeps a record."),*Name));});
  });
  for(int32 Who=0;Who<3;++Who)
   E.AddFreeform(G,FString::Printf(TEXT("Visit at %s: %s"),*Name,TakerNames[Who]),TEXT("03:12, at the marks, it comes now and can take you"),[this,ToMarks,Id,Name,Who]
   {
    ToMarks(Id,[this,Name,Who]
    {
     auto* F=Session();if(!F||!F->FarmSite)return;
     F->FarmSite->TestVisit(Who);
     Tell(true,FString::Printf(TEXT("The warning, then the %s comes at %s. The truck will not start. Lamp and noise change what it does."),TakerNames[Who],*Name),8);
    });
   });
  E.AddFreeform(G,FString::Printf(TEXT("Visit at %s by the night's own dice"),*Name),TEXT("03:12, at the marks; 12 seconds, it picks who comes"),[this,ToMarks,Id,Name]
  {
   ToMarks(Id,[this,Name]
   {
    auto* F=Session();if(!F||!F->FarmSite)return;
    F->FarmSite->TestReadyVisit();
    Tell(true,FString::Printf(TEXT("Stay at %s for 12 seconds, on foot, in the dark. Who comes is the rule's own draw."),*Name),8);
   });
  });
 }
 E.AddFreeform(G,TEXT("Second call: the radio about the marks"),TEXT("marks at the farm stand; the radio says so"),[this,ToMarks]
 {
  ToMarks(TEXT("farm"),[this]
  {
   auto* F=Session();if(!F||!F->FarmSite)return;
   F->FarmSite->TestReadyVisit();
   if(M->FieldRadio)M->FieldRadio->TestOwn();
   Tell(F->FarmSite->WakeCall(),TEXT("The second call, on the set, about the marks at the Alderete place."));
  });
 });
 E.AddFreeform(G,TEXT("Field visit: alone out in the open"),TEXT("03:12, past the fence, on foot; 60 seconds, then it comes"),[this]
 {
  auto* F=Session();if(!F||!F->FarmSite){Tell(false,TEXT("Freeform only"));return;}
  // The live rules are what it asks for, and a night that is neither its first nor calm.
  F->TestLiveRules();
  int32 Night=FMath::Max(2,F->Day());
  while(F->QuietNight(Night)&&Night<F->Day()+5)++Night;
  FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Day=Night;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(150,240);};
  Go(Stage,[this]
  {
   auto* S=Session();if(!S||!S->FarmSite)return;
   S->FarmSite->TestFieldRoll(0.f);
   Tell(true,TEXT("Stay out here on foot for 60 seconds. The night's own draw is set to yes: then it comes, without any marks."),8);
  });
 });
 // A preview, like those that were here: the night's own sky has its own schedule.
 E.AddFreeform(TEXT("Sky"),TEXT("High lone track"),TEXT("the night's 22 percent, now; outdoors; about 4 minutes"),[this]
 {
  const bool bOn=M&&M->SkyVisitors&&M->SkyVisitors->DevHighTrack();
  if(!bOn){Tell(false,TEXT("Freeform only"));return;}
  Tell(true,TEXT("One point of light, very high, for four minutes. Look up. From outdoors only."));
 });
}

// ---- the highway ----
void FS47DevFreeform::StartHighway(bool bNorth,bool bFollow)
{
 auto* F=Session();
 if(!F||!M->HighwayUfo||!M->World){Tell(false,TEXT("Freeform only"));return;}
 ClearRunning();
 ToHour(SmallFrom,SmallTo);
 // Clears its rest, which the encounter asks for: the save it holds is only this test run's.
 M->HighwayUfo->Begin(nullptr);
 // Its own lane either way, ahead of the open stretch.
 const double StartX=bNorth?1.8:-1.8,StartZ=bNorth?1700.:620.;
 FS47TripOpts O;O.AtPos=FVector(8000+StartX,S47RoadTerrain::SurfaceY(StartX,StartZ),StartZ);O.AtHeading=bNorth?0.:UE_DOUBLE_PI;O.Leg=TEXT("road");
 M->World->StartTrip(TEXT("road"),TEXT("diner"),O);
 M->World->TestInput=[this]{FS47DriveIn I;auto* D=M&&M->World?M->World->CabDrive():nullptr;if(D)I.Throttle=D->Speed<14?1:0;return I;};
 WaitFor([this]{auto* D=M->World?M->World->CabDrive():nullptr;return D&&M->World->bDriving&&!M->World->bBusy&&D->Speed>1;},12,[this,bFollow,bNorth]
 {
  if(M->World)M->World->TestInput=nullptr;
  if(!M->HighwayUfo||!M->HighwayUfo->Force(bFollow)){Tell(false,TEXT("the road is not clear for it: another encounter, or no room ahead"));return;}
  Tell(true,FString::Printf(TEXT("%s, 03:12. %s You are driving: steering and brakes are yours."),bNorth?TEXT("North"):TEXT("South"),bFollow?TEXT("The light follows you."):TEXT("A sheriff's car comes after the light.")),8);
 },[this]{if(M->World)M->World->TestInput=nullptr;Tell(false,TEXT("the truck did not get going on the road"));});
}

void FS47DevFreeform::AddHighway(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Highway");
 for(const bool bNorth:{true,false})for(const bool bFollow:{false,true})
  E.AddFreeform(G,FString::Printf(TEXT("%s: %s"),bNorth?TEXT("North"):TEXT("South"),bFollow?TEXT("the light follows you"):TEXT("sheriff follows the light")),TEXT("03:12, in the truck at speed on the open road"),[this,bNorth,bFollow]{StartHighway(bNorth,bFollow);});
 E.AddFreeform(G,TEXT("On the road, the night's own rule"),TEXT("03:12, north-bound; it comes on the open stretch unless it has had its three nights"),[this]
 {
  auto* F=Session();if(!F||!M->World){Tell(false,TEXT("Freeform only"));return;}
  ClearRunning();ToHour(SmallFrom,SmallTo);M->HighwayUfo->Begin(nullptr);
  const double StartX=1.8,StartZ=1700.;
  FS47TripOpts O;O.AtPos=FVector(8000+StartX,S47RoadTerrain::SurfaceY(StartX,StartZ),StartZ);O.AtHeading=0.;O.Leg=TEXT("road");
  M->World->StartTrip(TEXT("road"),TEXT("diner"),O);
  Tell(true,TEXT("You have the truck. Drive north: the open stretch is about 50 m ahead."),6);
 });
}

// ---- taken ----
void FS47DevFreeform::AddTaken(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Taken: every way");
 for(int32 Who=0;Who<3;++Who)
 {
  const FString Name=TakerNames[Who];
  E.AddFreeform(G,FString::Printf(TEXT("%s: a light over you, on foot"),*Name),TEXT("03:12, yard; 8 seconds to read it and go"),[this,Who,Name]
  {
   FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Who,Name]
   {
    auto* F=Session();if(!F||!F->Taken||!M->Player)return;
    const FVector Over(M->Player->Pos.X-FMath::Sin(M->Player->Yaw)*22,M->Player->FloorY,M->Player->Pos.Z-FMath::Cos(M->Player->Yaw)*22);
    if(!F->Taken->BeginOver(ES47Taker(Who),TEXT("the yard"),Over)){Tell(false,TEXT("already taken, or under a roof"));return;}
    Tell(true,TEXT("A light stands ahead of you, and in 8 seconds it starts to walk. Distance, a wall or waiting it out gets you clear."));
   });
  });
  E.AddFreeform(G,FString::Printf(TEXT("%s: through the roof, indoors"),*Name),TEXT("03:12, control room; steps, then the light"),[this,Who,Name]
  {
   FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Area=TEXT("saro");Stage.Where=[]{return ControlRoom();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Who,Name]
   {
    auto* F=Session();if(!F||!F->Taken)return;
    if(!F->Taken->Begin(ES47Taker(Who),TEXT("the control room"))){Tell(false,TEXT("being taken already"));return;}
    Tell(true,TEXT("Steps over the roof, then the light through the window. A wall away from it gets you clear."));
   });
  });
  E.AddFreeform(G,FString::Printf(TEXT("%s: reached by a body"),*Name),TEXT("03:12, yard; past any warning, straight to losing the time"),[this,Who,Name]
  {
   FStage Stage;Stage.From=SmallFrom;Stage.To=SmallTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,Who,Name]
   {
    auto* F=Session();if(!F||!F->Taken)return;
    const auto& Terms=FS47Taken::Terms(ES47Taker(Who));
    if(!F->Taken->TakeNow(ES47Taker(Who),TEXT("the yard"))){Tell(false,TEXT("being taken already"));return;}
    Tell(true,FString::Printf(TEXT("Taken now by the %s: %.0f to %.0f minutes gone."),*Name,Terms.MinMinutes,Terms.MaxMinutes));
   });
  });
 }
}

// ---- afterwards ----
void FS47DevFreeform::StartFieldTest(int32 Test)
{
 auto* F=Session();
 if(!F||!F->CaseField||!DevRun()){Tell(false,DevRun()?TEXT("Freeform only"):NoDevRun);return;}
 ClearRunning();
 const auto* Visitors=F->CaseFile().Get(2);
 // The case has to be open for a test to be planned: this test run's board gets one.
 if(!Visitors||!Visitors->bOpen)
 {
  int32 Seed=1;FS47FreeformCases Cases;for(;Seed<1000;++Seed){Cases.Reset(Seed);if(Cases.Get(2)->Truth==1)break;}
  if(Seed>=1000||!Cases.Open(2,1,F->Clock())){Tell(false,TEXT("the case could not be opened here"));return;}
  auto State=S47J::Read(S47J::Write(F->Save()));S47J::Set(State,TEXT("jobSeed"),Seed);S47J::Set(State,TEXT("tutorial"),0);S47J::Set(State,TEXT("cases"),Cases.Save());
  F->Begin(State,false);
 }
 FStage Stage;Stage.From=20;Stage.To=23;Stage.Area=TEXT("saro");
 Stage.Where=[Test]{return Test==0?FVector2D(5.25,-5.12):FVector2D(-1.,-20.);};
 Go(Stage,[this,Test]
 {
  auto* S=Session();if(!S||!S->CaseField)return;
  if(!S->PlanCaseTest(2,Test)&&S->PlannedFieldTest(2)!=Test){Tell(false,TEXT("that case will not plan this test now"));return;}
  if(!S->CaseField->Prepared()&&!S->CaseField->Prepare(Test)){Tell(false,TEXT("not close enough to prepare it, or it is already spoiled"));return;}
  double From=0,To=0;
  if(!S->CaseField->Window(From,To)){Tell(false,TEXT("prepared, but no window came of it"));return;}
  S->TestAdvance(FMath::Max(0.,From+60-S->Clock()));
  Tell(true,FString::Printf(TEXT("%s, prepared. The window is on now, until %s. It is measured from where you stand."),Test==0?TEXT("Roof listening"):TEXT("The gate"),*ClockText(To)),8);
 });
}

void FS47DevFreeform::AddAfter(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Afterwards and leads");
 for(int32 Who=0;Who<3;++Who)
  E.AddFreeform(G,FString::Printf(TEXT("After being taken by the %s"),TakerNames[Who]),TEXT("OPS clock stuck for a day; a line in your notes you did not write (none for Roswell)"),[this,Who]
  {
   auto* F=Session();if(!F||!F->Aftermath){Tell(false,TEXT("Freeform only"));return;}
   F->Aftermath->TestDice(0.f,1.f);
   F->Aftermath->AfterTaken(Who,F->Clock()-2*3600.);
   F->Aftermath->TestDice(-1.f,-1.f);
   Tell(true,F->Aftermath->LastLine().IsEmpty()?FString(TEXT("OPS stands at the last time you are sure of, until you have been on it a few seconds. No new line.")):TEXT("OPS stands at the last time you are sure of. And in your notes: ")+F->Aftermath->LastLine(),8);
  });
 E.AddFreeform(G,TEXT("A cow gone from the pasture"),TEXT("the next morning at 08:30, at the pasture"),[this]
 {
  auto* F=Session();if(!F||!F->Aftermath||!F->FarmSite){Tell(false,TEXT("Freeform only"));return;}
  F->Aftermath->TestDice(1.f,0.f);F->Aftermath->AfterFarmVisit(F->Clock());F->Aftermath->TestDice(-1.f,-1.f);
  if(!F->Aftermath->CowPlanned()){Tell(false,TEXT("two have gone already, or one is out"));return;}
  FStage Stage;Stage.From=8.5;Stage.To=11;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(60,240);};
  Go(Stage,[this]{Tell(true,TEXT("One of the herd is gone for some days. Count them; a line goes on the task list when you notice."));});
 });
 E.AddFreeform(G,TEXT("Marks by daylight"),TEXT("marks left in the night, seen at 09:00"),[this]
 {
  auto* F=Session();if(!F||!F->FarmSite){Tell(false,TEXT("Freeform only"));return;}
  FStage Night;Night.From=SmallFrom;Night.To=SmallTo;Night.Area=TEXT("saro");
  Go(Night,[this]
  {
   auto* S=Session();if(!S||!S->FarmSite)return;
   S->FarmSite->ArmFarm(S->Clock());
   const FVector C=S->FarmSite->Centre();
   ToHour(9,11.9);M->Player->Place(C.X+6,C.Z+6,0);M->Player->Pitch=0;AimAt(C);
   Tell(true,TEXT("The same marks by day, until noon. E on them: what they are in daylight, fixed for this place and day."));
  });
 });
 E.AddFreeform(G,TEXT("The night's mystery: a trace of it"),TEXT("02:40, night two or later, control room; a dial kicks, the rack hums"),[this]
 {
  auto* F=Session();if(!F||!F->Director){Tell(false,TEXT("Freeform only"));return;}
  const auto* Case=F->CaseFile().Get(FS47FreeformCases::MysteryLine);
  if(!Case||Case->bOpen){Tell(false,TEXT("that case is already open in this run; the mystery only comes until then"));return;}
  FStage Stage;Stage.From=2.67;Stage.To=5;Stage.Day=2;Stage.Area=TEXT("saro");Stage.Where=[]{return ControlRoom();};
  Go(Stage,[this]
  {
   auto* S=Session();if(!S||!S->Director)return;
   auto& D=*S->Director;
   D.TestMysteryDice(1.f);D.TestSchedule(0.f,-1,40.);ScheduleHeld=20;
   WaitFor([&D]{return D.MysteryNoticeClue()>=0||D.MysteryTrailOpen();},45,[this]
   {
    auto* T=Session();if(T&&T->Director){T->Director->TestMysteryDice(bNatural?.3f:-1.f);T->Director->TestSchedule(-1.f,-1,1.);}
    ScheduleHeld=0;
    Tell(true,TEXT("The rack hums and a dial kicks. It is on the case board; the other place stays worth a look until noon."));
   },[this]
   {
    auto* T=Session();if(T&&T->Director){T->Director->TestMysteryDice(bNatural?.3f:-1.f);T->Director->TestSchedule(-1.f,-1,1.);}
    ScheduleHeld=0;
    Tell(false,TEXT("nothing showed in 45 seconds: another event or its rest had the moment. Stop everything, then try again"));
   });
  });
 });
 E.AddFreeform(G,TEXT("Woken from sleep"),TEXT("22:00, night two, marks standing; goes to rest, and something wakes you"),[this]
 {
  auto* F=Session();if(!F||!F->FarmSite){Tell(false,TEXT("Freeform only"));return;}
  FStage Stage;Stage.From=22;Stage.To=23;Stage.Day=2;Stage.Area=TEXT("saro");Stage.Where=[]{return ControlRoom();};
  Go(Stage,[this]
  {
   auto* S=Session();if(!S||!S->FarmSite)return;
   S->TestAllowWake();S->FarmSite->ArmFarm(S->Clock());S->FarmSite->TestReadyVisit();
   if(!S->Rest()){Tell(false,TEXT("rest was refused: a recording is running, or something is close by"));return;}
   Tell(true,FString::Printf(TEXT("You lay down at 22:00. Something woke you at %s."),*ClockText(S->Clock())),8);
  });
 });
 E.AddFreeform(G,TEXT("Field test: roof listening"),TEXT("20:00, at the ladder; replaces this test run's case board if the case is not open"),[this]{StartFieldTest(0);});
 E.AddFreeform(G,TEXT("Field test: the sealed gate"),TEXT("20:00, at the gate; replaces this test run's case board if the case is not open"),[this]{StartFieldTest(1);});
 E.AddFreeform(G,TEXT("Give me the field radio"),TEXT("what breaks up when something is near, and carries the tips"),[this]
 {
  if(M&&M->FieldRadio)M->FieldRadio->TestOwn();
  Tell(true,TEXT("You have the field radio. It breaks up when something comes near."));
 });
 // What has no line of its own, and why: said here rather than left out.
 static const TCHAR* const Not=TEXT("No line of their own, and why");
 const auto Say=[this](const TCHAR* Text){Tell(true,Text,8);};
 E.AddFreeform(Not,TEXT("Day cards"),TEXT("three a day, from the clock: use Time, Next day"),[Say]{Say(TEXT("Day cards come with the day. Time, Next day gives three more."));});
 E.AddFreeform(Not,TEXT("The worn array"),TEXT("a new station's state, not an event; repairing it is play"),[Say]{Say(TEXT("15 of 27 down from the start of a new station. Repairing them is the work itself."));});
 E.AddFreeform(Not,TEXT("Radio near danger"),TEXT("follows the danger: Give me the field radio, then any figure"),[Say]{Say(TEXT("Give yourself the field radio, then start a figure or a visit."));});
 E.AddFreeform(Not,TEXT("Door marks"),TEXT("left by a door visit's ending"),[Say]{Say(TEXT("Start a door visit; each ending leaves its own marks by the door until noon."));});
 E.AddFreeform(Not,TEXT("First pulse's ticks"),TEXT("the first pulsar anyone records; set by Natural rhythm in a new run"),[Say]{Say(TEXT("It comes on the first pulsar card recorded, with Natural rhythm on."));});
}

// ---- running the night ----
void FS47DevFreeform::SetNatural(bool bOn)
{
 auto* F=Session();if(!F||!F->Director)return;
 auto& D=*F->Director;
 bNatural=bOn;
 if(M->Rhythm)M->Rhythm->SetWatching(bOn||bPanelWanted);
 if(bOn)
 {
  // What a developer run switches off (HENDELSER.md), through the same switches the scenario tests use. A run cannot be put
  // back to fixed rules once the live ones are on: those live in the session, and a new test run starts clean.
  F->TestLiveRules();F->TestEveningGate();F->TestFirstPulse();F->TestAllowWake();
  D.TestOrbNight(true);D.TestMysteryDice(.3f);D.TestSchedule(-1.f,-1,Speed);
  ShowPanel(true);
  if(Menu)Menu->Relabel(TEXT("Panel: off"),TEXT("Panel: short"));
 }
 else
 {
  D.TestOrbNight(false);D.TestMysteryDice(-1.f);D.TestCarDice(-1.f);D.TestSchedule(-1.f,-1,1.);D.TestEventPace(1.);
  if(F->Aftermath)F->Aftermath->TestDice(-1.f,-1.f);
  Speed=1;SpeedUsed=1;Holding.Empty();
 }
 UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 natural rhythm %s, speed x%.0f, day %d %s"),bOn?TEXT("on"):TEXT("off"),Speed,F->Day(),*ClockText(F->Clock()));
}

FString FS47DevFreeform::HoldFor() const
{
 auto* F=Session();if(!F)return FString();
 if(F->Taken&&F->Taken->Busy())return TEXT("taken");
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())return TEXT("site visit");
 if(F->IndoorDread&&F->IndoorDread->DoorActive())return TEXT("door");
 if(F->IndoorDread&&F->IndoorDread->PeekActive())return TEXT("window");
 if(M->HighwayUfo&&M->HighwayUfo->StageName()!=TEXT("idle")&&M->HighwayUfo->StageName()!=TEXT("done"))return TEXT("highway");
 if(F->Director&&F->Director->Busy())
 {
  // What has nothing to see runs on: an instrument waits for the next reading, a fault for its repair.
  const FString& Id=F->Director->ActiveEvent();
  static const TCHAR* const Quiet[]={TEXT("extra_pulse"),TEXT("wrong_source"),TEXT("time_disagreement"),TEXT("duplicate_print"),TEXT("unsigned_telex"),TEXT("s03_failure"),TEXT("feeder_fault"),TEXT("landing_trace"),TEXT("empty_car"),TEXT("roomlight_return"),TEXT("service_door_open")};
  for(const TCHAR* Q:Quiet)if(Id==Q)return FString();
  return Id;
 }
 return FString();
}

void FS47DevFreeform::StepStand()
{
 auto* F=Session();
 if(StandPlace.IsEmpty()||!F||!M->Player||!M->Web||!M->World||M->World->bDriving||M->World->bBusy||M->Web->Area()!=TEXT("saro")||!HoldFor().IsEmpty())return;
 const double Hour=FMath::Fmod(F->Clock(),86400.)/3600.;
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(Hour<21||StandNight==Night)return;
 StandNight=Night;
 const FVector House=S47FarmHouse::Point(6,4.4)+FVector(-24,0,300);
 if(StandPlace==TEXT("house")){M->Player->Place(House.X,House.Z,0);M->Player->Pitch=0;}
 else
 {
  // Out on the open ground past the farm, on foot: somewhere the land lets them stand.
  bool bPut=false;
  for(int32 I=0;I<24&&!bPut;++I)
  {
   const double A=I*UE_DOUBLE_PI/12,R=45+(I%3)*15;const double X=House.X+FMath::Cos(A)*R,Z=House.Z+FMath::Sin(A)*R;
   if(!M->Player->Walkable(X,Z)||(M->Player->OpenLand&&!M->Player->OpenLand(X,Z))||M->World->Indoors(FVector(X,M->Player->FloorY+1,Z)))continue;
   M->Player->Place(X,Z,A+UE_DOUBLE_PI);M->Player->Pitch=0;bPut=true;
  }
  if(!bPut)M->Player->Place(House.X+50,House.Z+20,0);
 }
 UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 natural night %d: standing at %s, %.0f %.0f, %s"),Night,*StandPlace,M->Player->Pos.X,M->Player->Pos.Z,*ClockText(F->Clock()));
}

void FS47DevFreeform::StepNatural(double Dt)
{
 auto* F=Session();if(!F||!F->Director)return;
 auto& D=*F->Director;
 // The two the session cannot be told once for all: a real draw each time one is asked.
 D.TestCarDice(FMath::FRand());
 if(F->Aftermath)F->Aftermath->TestDice(FMath::FRand(),FMath::FRand());
 const FString Why=bHold?HoldFor():FString();
 if(Why!=Holding)
 {
  if(!Why.IsEmpty()&&Speed>1&&M->UI)M->UI->Toast(FString::Printf(TEXT("Natural night: x1 for %s."),*Why),3);
  Holding=Why;
 }
 double Use=Why.IsEmpty()?Speed:1;
 // A formation is worth seeing, but it should not cost a night.
 if(M->SkyVisitors&&M->SkyVisitors->FormationData().bActive&&Use>5)Use=5;
 SpeedUsed=Use;
 if(ScheduleHeld>0)ScheduleHeld-=Dt;else D.TestSchedule(-1.f,-1,Use);
 // What runs on at speed ages at speed: its minutes are the night's minutes, as they are at the wheel. What is held at x1 is.
 D.TestEventPace(Why.IsEmpty()?Use:1.);
 if(Use>1&&Dt>0&&Dt<.5&&!F->Held())
 {
  const double Now=F->Clock();
  F->TestAdvance(FMath::Max(0.,FS47FreeformSession::AdvanceClock(Now,Dt*(Use-1))-Now));
 }
}

void FS47DevFreeform::ShowPanel(bool bShow)
{
 bPanelWanted=bShow;
 if(!GEngine||!GEngine->GameViewport)return;
 if(bShow&&!Panel)
 {
  Panel=SNew(SS47DevPanel).Owner(this);
  GEngine->GameViewport->AddViewportWidgetContent(Panel.ToSharedRef(),26);
 }
 else if(!bShow&&Panel)
 {
  GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
  Panel.Reset();
 }
}

FString FS47DevFreeform::PanelText() const
{
 auto* F=Session();
 if(!F)return TEXT("DIRECTOR  ikke i Freeform");
 FString T=M->Rhythm?M->Rhythm->PanelText(bPanelFull):FString();
 if(bNatural)T+=FString::Printf(TEXT("\nNATURLIG  x%.0f%s"),SpeedUsed,Holding.IsEmpty()?TEXT(""):*FString::Printf(TEXT("  (x1 for %s)"),*Holding));
 return T;
}

FString FS47DevFreeform::MenuNote() const
{
 auto* F=Session();
 FString T=TEXT("Each line sets the clock and the place, and says what it does. Nothing is saved.");
 if(!F)return T;
 T=FString::Printf(TEXT("Day %d, %s, %s. %s"),F->Day(),*ClockText(F->Clock()),M->Web?*M->Web->Area():TEXT(""),*T);
 if(!DevRun())T+=TEXT(" THIS IS YOUR OWN CASE: most lines need a test run. Run the night, Go to a test run.");
 if(bNatural)T+=FString::Printf(TEXT(" Natural rhythm is on at x%.0f."),Speed);
 return T;
}

void FS47DevFreeform::AddRunNight(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Run the night");
 E.AddFreeform(G,TEXT("Natural rhythm: off"),TEXT("the rules of a real game, here, with nothing saved: what a developer run otherwise switches off"),[this]
 {
  const bool bOn=!bNatural;
  if(!Session())return;
  SetNatural(bOn);
  if(Menu)Menu->Relabel(bOn?TEXT("Natural rhythm: off"):TEXT("Natural rhythm: ON"),bOn?TEXT("Natural rhythm: ON"):TEXT("Natural rhythm: off"));
  if(Menu)Menu->Open();
 });
 E.AddFreeform(G,TEXT("Speed: x1"),TEXT("the clock and the Director's schedule, cycles x1, x5, x15, x30, x60. A night is 45 minutes at x1, 3 at x15"),[this]
 {
  int32 I=0;for(int32 N=0;N<UE_ARRAY_COUNT(SpeedValues);++N)if(SpeedValues[N]==Speed)I=N;
  const FString Was=FString::Printf(TEXT("Speed: %s"),SpeedLabels[I]);
  I=(I+1)%UE_ARRAY_COUNT(SpeedValues);Speed=SpeedValues[I];
  if(Speed>1&&!bNatural)
  {
   SetNatural(true);
   if(Menu)Menu->Relabel(TEXT("Natural rhythm: off"),TEXT("Natural rhythm: ON"));
  }
  if(Menu){Menu->Relabel(Was,FString::Printf(TEXT("Speed: %s"),SpeedLabels[I]));Menu->Open();}
 });
 E.AddFreeform(G,TEXT("Back to x1 for events: on"),TEXT("the speed drops while there is something to see"),[this]
 {
  const FString Was=bHold?TEXT("Back to x1 for events: on"):TEXT("Back to x1 for events: off");
  bHold=!bHold;
  if(Menu){Menu->Relabel(Was,bHold?TEXT("Back to x1 for events: on"):TEXT("Back to x1 for events: off"));Menu->Open();}
 });
 E.AddFreeform(G,TEXT("Panel: off"),TEXT("what can happen now, what is next, and why not; cycles off, short, full"),[this]
 {
  const FString Was=!bPanelWanted?TEXT("Panel: off"):bPanelFull?TEXT("Panel: full"):TEXT("Panel: short");
  if(!bPanelWanted){bPanelFull=false;ShowPanel(true);}
  else if(!bPanelFull)bPanelFull=true;
  else{bPanelFull=false;ShowPanel(false);}
  if(M->Rhythm)M->Rhythm->SetWatching(bPanelWanted||bNatural);
  if(Menu){Menu->Relabel(Was,!bPanelWanted?TEXT("Panel: off"):bPanelFull?TEXT("Panel: full"):TEXT("Panel: short"));Menu->Open();}
 });
 E.AddFreeform(G,TEXT("Copy the panel to the log"),TEXT("every line, why and all; S47: rhythm panel"),[this]{if(M->Rhythm){M->Rhythm->LogPanel();Tell(true,TEXT("The panel is in the log."),3);}});
 E.AddFreeform(G,TEXT("Load the rhythm file again"),TEXT("Content/Director/director.json; a file that does not parse gives the standard values"),[this]{if(M->Rhythm){M->Rhythm->Load();Tell(true,FString::Printf(TEXT("Rhythm from: %s."),*M->Rhythm->Source()),4);}});
 E.AddFreeform(G,TEXT("Use the override file"),TEXT("Saved/Director/director.override.json, by hand, in a test run; your own game needs -S47DirectorOverride"),[this]
 {
  if(!M->Rhythm)return;
  if(!DevRun()){Tell(false,TEXT("only in a test run. Your own game needs the start flag -S47DirectorOverride."));return;}
  FString Why;
  if(M->Rhythm->LoadOverride(Why))Tell(true,TEXT("OVERRIDE ACTIVE. Load the rhythm file again to leave it."),5);else Tell(false,Why);
 });
 E.AddFreeform(G,TEXT("Stop everything"),TEXT("whatever is running is over; the night goes on"),[this]{ClearRunning();Tell(true,TEXT("Everything running has been stopped."),3);});
 E.AddFreeform(G,TEXT("Go to a test run"),TEXT("a new Freeform developer run in this process; your case stays as it was last saved"),[this]
 {
  if(DevRun()){Tell(true,TEXT("This already is a test run."),3);return;}
  if(!M->Flow)return;
  US47Flow::FStart S;S.bDev=true;S.bFreeform=true;S.CaseId=0;
  M->Flow->StartGame(S);
 });
}

void FS47DevFreeform::AddTimePlace(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Time and place");
 struct FHour{const TCHAR* Label;double Hour;};
 static const FHour Hours[]={{TEXT("Clock: 08:00"),8},{TEXT("Clock: 18:40, dusk"),18.66},{TEXT("Clock: 19:00, the shift"),19},{TEXT("Clock: 21:30"),21.5},{TEXT("Clock: 00:30"),.5},{TEXT("Clock: 01:00"),1},{TEXT("Clock: 02:00"),2},{TEXT("Clock: 03:12"),SmallFrom},{TEXT("Clock: 05:50"),5.83}};
 for(const FHour& H:Hours)
 {
  const double Hour=H.Hour;const FString Label=H.Label;
  E.AddFreeform(G,H.Label,TEXT("forward to the next one"),[this,Hour,Label]
  {
   auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
   ToHour(Hour,Hour+.01);Tell(true,FString::Printf(TEXT("Day %d, %s."),F->Day(),*ClockText(F->Clock())),3);
  });
 }
 E.AddFreeform(G,TEXT("Mend the power"),TEXT("a feeder fault left by an earlier line, as if repaired; the array's own fault wants its repair"),[this]
 {
  auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
  F->SetPowerFault(false);Tell(true,TEXT("The power is back."),3);
 });
 E.AddFreeform(G,TEXT("Clock: +1 hour"),TEXT("forward"),[this]{auto* F=Session();if(!F)return;F->TestAdvance(3600);Tell(true,FString::Printf(TEXT("Day %d, %s."),F->Day(),*ClockText(F->Clock())),3);});
 E.AddFreeform(G,TEXT("Next day, same hour"),TEXT("new day cards, new draws"),[this]{auto* F=Session();if(!F)return;F->TestAdvance(86400);Tell(true,FString::Printf(TEXT("Day %d, %s."),F->Day(),*ClockText(F->Clock())),3);});
 struct FPlace{const TCHAR* Label;const TCHAR* Area;double X,Z,Yaw;};
 static const FPlace Places[]={{TEXT("Go: control room"),TEXT("saro"),0,-1.2,UE_DOUBLE_PI},{TEXT("Go: SARO yard"),TEXT("saro"),7,19,UE_DOUBLE_PI},{TEXT("Go: among the antennas"),TEXT("saro"),-3.25,-29,0},{TEXT("Go: Alderete yard"),TEXT("saro"),84,510,0},{TEXT("Go: the pasture"),TEXT("saro"),60,240,0},{TEXT("Go: past the fence"),TEXT("saro"),150,240,0},{TEXT("Go: Caliche street"),TEXT("diner"),7995,2250,0}};
 for(const FPlace& P:Places)
 {
  FStage Stage;Stage.Area=P.Area;const double X=P.X,Z=P.Z;Stage.Where=[X,Z]{return FVector2D(X,Z);};Stage.Yaw=P.Yaw;
  const FString Label=P.Label;
  E.AddFreeform(G,P.Label,TEXT("on foot; whatever is running stops"),[this,Stage,Label]{Go(Stage,[this,Label]{Tell(true,Label,3);});});
 }
}

// ---- out on the land, what the shops have, and the radio ----
void FS47DevFreeform::ToLandPlace(const FString& Id,double Off,double From,double To,TFunction<void(double,double)> Then)
{
 auto* F=Session();
 double X=0,Z=0;
 if(!F||!F->Places||!F->Places->Spot(Id,X,Z)){Tell(false,TEXT("that place is not in this scene"));return;}
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const double WorldX=Road.X+X,WorldZ=Road.Z+Z;
 FStage Stage;Stage.From=From;Stage.To=To;Stage.Area=TEXT("road");Stage.Where=[WorldX,WorldZ,Off]{return FVector2D(WorldX,WorldZ+Off);};Stage.Yaw=0;
 Go(Stage,[Then,WorldX,WorldZ]{Then(WorldX,WorldZ);});
}

void FS47DevFreeform::AddLand(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Places out on the land");
 for(const FS47Places::FDef& D:FS47Places::Defs())
 {
  const FString Id=D.Id,Title=D.Title;
  if(Id==TEXT("motel"))continue;
  E.AddFreeform(G,FString::Printf(TEXT("Go: %s"),*Title),TEXT("on foot, in front of it, the clock as it is; E on it finds it"),[this,Id,Title]
  {
   ToLandPlace(Id,5,-1,-1,[this,Title](double X,double Z){AimAt(FVector(X,M->Player->FloorY+1,Z));Tell(true,FString::Printf(TEXT("At %s. E on it finds it, and marks the map."),*Title),5);});
  });
  E.AddFreeform(G,FString::Printf(TEXT("Lights over %s"),*Title),TEXT("22:00, 110 m off, looking at them; they leave marks"),[this,Id,Title]
  {
   ToLandPlace(Id,110,EveningFrom,EveningTo,[this,Id,Title](double,double)
   {
    auto* F=Session();if(!F||!F->Director)return;
    if(!F->Director->ForceLights(Id)){Tell(false,F->Director->LightPlacePoint(Id).IsZero()?FString::Printf(TEXT("lights do not come to %s: it is not among the Director's places for them"),*Title):WhyRefused(TEXT("town_lights")));return;}
    AimAt(F->Director->EventAnchor());
    Tell(true,FString::Printf(TEXT("Lights low over %s. They go when you come within 70 m, and leave marks."),*Title));
   });
  });
  E.AddFreeform(G,FString::Printf(TEXT("Marks at %s, and what comes"),*Title),TEXT("03:12, at the marks; 12 seconds on foot, then the night's own draw of who"),[this,Id,Title]
  {
   ToLandPlace(Id,8,SmallFrom,SmallTo,[this,Id,Title](double,double)
   {
    auto* F=Session();if(!F||!F->FarmSite||!F->Director)return;
    const FVector Point=F->Director->LightPlacePoint(Id);
    if(Point.IsZero()){Tell(false,FString::Printf(TEXT("no marks at %s: it is not among the Director's places for lights"),*Title));return;}
    F->FarmSite->Arm(F->Clock(),Id,Title,M->Web->Area(),Point);F->FarmSite->TestReadyVisit();
    const FVector C=F->FarmSite->Centre();M->Player->Place(C.X+6,C.Z+6,0);M->Player->Pitch=0;AimAt(C);
    Tell(true,FString::Printf(TEXT("Marks at %s. Stay 12 seconds on foot, in the dark, and something comes."),*Title),7);
   });
  });
 }
 static const TCHAR* const K=TEXT("What the shops have, and the radio");
 E.AddFreeform(K,TEXT("Give me everything on offer"),TEXT("spotlight, cot, magnetometer, second logger, film, alarm; this test run only"),[this]
 {
  auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
  for(const auto& O:FS47Openings::Defs())F->TestGiveKit(O.Id);
  Tell(true,TEXT("All of it is yours. The spotlight is F in the cab; the cot is at a place on the land; the alarm is in the yard."),6);
 });
 for(const bool bOut:{false,true})
  E.AddFreeform(K,bOut?TEXT("Yard alarm trips, and is put out"):TEXT("Yard alarm trips"),bOut?TEXT("22:00, yard; then it wants a fuse at its box"):TEXT("22:00, yard; lights and klaxon"),[this,bOut]
  {
   FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
   Go(Stage,[this,bOut]
   {
    auto* F=Session();if(!F||!F->YardAlarm){Tell(false,TEXT("Freeform only"));return;}
    F->TestGiveKit(TEXT("alarm"));
    F->YardAlarm->TestTrip(bOut?0.f:1.f);
    Tell(true,bOut?TEXT("The klaxon, and then it is out. A fuse at its box puts it right."):TEXT("Lights and klaxon: as if something had come within 48 m."));
   });
  });
 E.AddFreeform(K,TEXT("CB: the next line"),TEXT("the next that fits; each once in a game, and most mean nothing"),[this]
 {
  auto* F=Session();const bool bSaid=F&&F->Tips&&F->Tips->DevCb();
  if(!bSaid)Tell(false,TEXT("no CB line left that fits this game"));else{bLastOk=true;LastText=TEXT("a CB line");}
 });
 E.AddFreeform(K,TEXT("1420: the next caller"),TEXT("the next that fits; each once in a game"),[this]
 {
  auto* F=Session();const bool bSaid=F&&F->Tips&&F->Tips->DevAm();
  if(!bSaid)Tell(false,TEXT("it needs the truck's radio on 1420 (R in the cab), and a caller left that fits this game"));else{bLastOk=true;LastText=TEXT("a caller on 1420");}
 });
 const auto SleepUntilMorning=[this](FS47FreeformSession::ERest Where,const TCHAR* Say)
 {
  auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
  const double Next=(FMath::FloorToDouble((F->Clock()-8*3600.)/86400.)+1)*86400.+8*3600.;
  if(!F->RestAt(Next,Where)){Tell(false,TEXT("rest was refused: something is close by, or a recording is running"));return;}
  Tell(true,FString::Printf(TEXT("%s It is now day %d, %s."),Say,F->Day(),*ClockText(F->Clock())),6);
 };
 E.AddFreeform(K,TEXT("Sleep at the motel"),TEXT("until morning; nothing wakes you there"),[SleepUntilMorning]{SleepUntilMorning(FS47FreeformSession::ERest::Motel,TEXT("A night at the motel."));});
 E.AddFreeform(K,TEXT("Sleep on a cot, where you are"),TEXT("until morning; whatever comes by wakes you"),[SleepUntilMorning]{SleepUntilMorning(FS47FreeformSession::ERest::Field,TEXT("A night on the cot."));});
}

// ---- anywhere: every event at a place from the list ----
FString FS47DevFreeform::ChoosePlace(const FS47PlaceNeed& Need) const
{
 auto* F=Session();const FS47EventPlaces* Places=F?F->EventPlaces.Get():nullptr;if(!Places)return FString();
 if(!ChosenPlace.IsEmpty())
 {
  const FS47EventPlace* P=Places->Find(ChosenPlace);
  if(!P)return FString();
  const bool bFits=(Need.All==ES47PlaceTrait::None||P->Has(Need.All))&&(Need.Any==ES47PlaceTrait::None||P->HasAny(Need.Any));
  return bFits?ChosenPlace:FString();
 }
 return Places->Pick(Need,uint32(FPlatformTime::Cycles())).Id;
}

void FS47DevFreeform::ToPlace(const FString& Id,double Off,double From,double To,TFunction<void()> Then)
{
 auto* F=Session();const FS47EventPlaces* Places=F?F->EventPlaces.Get():nullptr;const FS47EventPlace* P=Places?Places->Find(Id):nullptr;
 if(!P){Tell(false,FString::Printf(TEXT("%s is not on the list of places"),*Id));return;}
 FString Area=FS47EventPlaces::AreaOf(*P);
 // On foot in Caliche is the diner's area.
 if(Area==TEXT("road")&&(P->Region==TEXT("caliche")||P->Region==TEXT("diner")))Area=TEXT("diner");
 const FVector At=S47OpenLand::FromRoad(Area,P->X,P->Z+Off);
 FStage Stage;Stage.From=From;Stage.To=To;Stage.Area=Area;Stage.Where=[At]{return FVector2D(At.X,At.Z);};Stage.Yaw=0;
 Go(Stage,MoveTemp(Then));
}

void FS47DevFreeform::AddAnywhere(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Anywhere: events at a place from the list");
 // Which place: random (the draw from where you stand), or each on the list in turn.
 E.AddFreeform(G,TEXT("Place: random"),TEXT("cycles through the list; random is the game's own draw from where you stand"),[this]
 {
  const TArray<FString> Ids=FS47EventPlaces::Ids();
  const int32 At=ChosenPlace.IsEmpty()?-1:Ids.IndexOfByKey(ChosenPlace);
  const FString Was=ChosenPlace.IsEmpty()?FString(TEXT("Place: random")):TEXT("Place: ")+ChosenPlace;
  ChosenPlace=At+1<Ids.Num()?Ids[At+1]:FString();
  const FString Now=ChosenPlace.IsEmpty()?FString(TEXT("Place: random")):TEXT("Place: ")+ChosenPlace;
  if(Menu){Menu->Relabel(Was,Now);Menu->Open();}
 });
 const auto Name=[this](const FString& Id){auto* F=Session();return F&&F->EventPlaces?F->EventPlaces->TitleOf(Id):Id;};
 E.AddFreeform(G,TEXT("Lights over it, from 110 m"),TEXT("22:00, on its ground, looking at them; they go when you come close, and leave marks"),[this,Name]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("town_lights");Need.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town|ES47PlaceTrait::Farm|ES47PlaceTrait::Array|ES47PlaceTrait::Lot;Need.FarShare=.35f;
  const FString Id=ChoosePlace(Need);if(Id.IsEmpty()){Tell(false,TEXT("lights do not come to that place"));return;}
  ToPlace(Id,110,EveningFrom,EveningTo,[this,Id,Name]
  {
   auto* F=Session();if(!F||!F->Director)return;
   if(!F->Director->ForceAt(TEXT("town_lights"),Id)){Tell(false,WhyRefused(TEXT("town_lights")));return;}
   AimAt(F->Director->EventAnchor());
   Tell(true,FString::Printf(TEXT("Lights low over %s. They leave marks when they go."),*Name(Id)));
  });
 });
 E.AddFreeform(G,TEXT("Lights over it, heard of at SARO"),TEXT("22:00, yard; a radio tip, a drive away if it is far"),[this,Name]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("town_lights");Need.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town|ES47PlaceTrait::Farm|ES47PlaceTrait::Array|ES47PlaceTrait::Lot;Need.MinM=150;Need.FarShare=.35f;
  FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.Area=TEXT("saro");Stage.Where=[]{return Yard();};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Need,Name]
  {
   auto* F=Session();if(!F||!F->Director)return;
   const FString Id=ChoosePlace(Need);
   if(!F->Director->ForceAt(TEXT("town_lights"),Id)){Tell(false,WhyRefused(TEXT("town_lights")));return;}
   Tell(true,FString::Printf(TEXT("The tip goes out: lights over %s."),*Name(F->Director->EventPlace())));
  });
 });
 E.AddFreeform(G,TEXT("Lights seen from where you stand"),TEXT("22:00, here; the draw picks a place a few hundred metres off on this ground"),[this,Name]
 {
  FStage Stage;Stage.From=EveningFrom;Stage.To=EveningTo;Stage.bStay=true;
  Go(Stage,[this,Name]
  {
   auto* F=Session();if(!F||!F->Director)return;
   if(!F->Director->ForceAt(TEXT("farm_lights"),FString())){Tell(false,WhyRefused(TEXT("farm_lights")));return;}
   AimAt(F->Director->EventAnchor());
   Tell(true,FString::Printf(TEXT("Lights low over %s, seen from here."),*Name(F->Director->EventPlace())));
  });
 });
 E.AddFreeform(G,TEXT("Marks at it, and what comes"),TEXT("03:12, at the marks; 12 seconds on foot, then the night's own draw of who"),[this,Name]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("marks");Need.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town|ES47PlaceTrait::Farm|ES47PlaceTrait::Array|ES47PlaceTrait::Lot;Need.FarShare=.35f;
  const FString Id=ChoosePlace(Need);if(Id.IsEmpty()){Tell(false,TEXT("no marks at that place"));return;}
  ToPlace(Id,14,SmallFrom,SmallTo,[this,Id,Name]
  {
   auto* F=Session();if(!F||!F->FarmSite||!F->Director||!F->EventPlaces||!M->Web)return;
   const FS47EventPlace* P=F->EventPlaces->Find(Id);if(!P)return;
   F->FarmSite->Arm(F->Clock(),Id,P->Title,FS47EventPlaces::AreaOf(*P),F->Director->LightPlacePoint(Id));F->FarmSite->TestReadyVisit();
   const FVector C=F->FarmSite->Centre();M->Player->Place(C.X+6,C.Z+6,0);M->Player->Pitch=0;AimAt(C);
   Tell(true,FString::Printf(TEXT("Marks at %s. Stay 12 seconds on foot, in the dark, and something comes."),*Name(Id)),7);
  });
 });
 E.AddFreeform(G,TEXT("Marked ground and fragment at it"),TEXT("11:00, on its ground; photograph it, then pick it up"),[this,Name]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("landing_trace");Need.All=ES47PlaceTrait::Open;Need.FarShare=.35f;
  const FString Id=ChoosePlace(Need);if(Id.IsEmpty()){Tell(false,TEXT("marked ground needs open ground: choose another place"));return;}
  ToPlace(Id,30,11,18,[this,Id,Name]
  {
   auto* F=Session();if(!F||!F->Director)return;
   if(!F->Director->ForceAt(TEXT("landing_trace"),Id)){Tell(false,WhyRefused(TEXT("landing_trace")));return;}
   AimAt(F->Director->EventAnchor());
   Tell(true,FString::Printf(TEXT("Marked ground and a fragment at %s, ahead of you."),*Name(Id)));
  });
 });
 const auto RoomOf=[](const FS47EventPlace& P){return P.DoorKey==TEXT("house")?FString(TEXT("house")):P.Id==TEXT("saro")?FString(TEXT("saro")):P.DoorKey;};
 E.AddFreeform(G,TEXT("Door visit there"),TEXT("03:12, in the room; something tries the door"),[this,RoomOf]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("door_visit");Need.All=ES47PlaceTrait::Door;Need.FarShare=.35f;
  const FString Id=ChoosePlace(Need);auto* F=Session();const FS47EventPlace* P=F&&F->EventPlaces?F->EventPlaces->Find(Id):nullptr;
  if(!P||RoomOf(*P).IsEmpty()){Tell(false,TEXT("that place has no door it comes to: choose one with a door (the old house, a room in town)"));return;}
  StartDoor(RoomOf(*P),false);
 });
 E.AddFreeform(G,TEXT("Window there"),TEXT("03:12, in the room; a head rises at a window or door you can see"),[this,RoomOf]
 {
  FS47PlaceNeed Need;Need.Kind=TEXT("peek");Need.All=ES47PlaceTrait::Window;Need.FarShare=.35f;
  const FString Id=ChoosePlace(Need);auto* F=Session();const FS47EventPlace* P=F&&F->EventPlaces?F->EventPlaces->Find(Id):nullptr;
  if(!P||RoomOf(*P).IsEmpty()){Tell(false,TEXT("no room to stand in there yet: SARO, the old house or a room in town"));return;}
  StartPeek(RoomOf(*P));
 });
 E.AddFreeform(G,TEXT("Low lights somewhere on the array"),TEXT("21:00, 18 m from them; where they come after the first time"),[this]
 {
  FStage Stage;Stage.From=21;Stage.To=23.2;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(FS47DuskOrb::HomeX,FS47DuskOrb::HomeZ+18);};
  Go(Stage,[this]
  {
   auto* F=Session();if(!F||!F->Director)return;
   if(!F->Director->ForceAt(TEXT("dusk_orb"),FString())){Tell(false,WhyRefused(TEXT("dusk_orb")));return;}
   const FVector A=F->Director->EventAnchor();M->Player->Place(A.X,A.Z+18,0);M->Player->Pitch=0;
   After(.3,[this]{auto* S=Session();if(S&&S->Director)AimAt(S->Director->EventAnchor());});
   Tell(true,TEXT("Low lights over another part of the array. They flee at 7 m."));
  });
 });
 E.AddFreeform(G,TEXT("Where the draw puts things now"),TEXT("from where you stand: nothing starts; the log has the list"),[this,Name]
 {
  auto* F=Session();if(!F||!F->Director||!F->EventPlaces){Tell(false,TEXT("Freeform only"));return;}
  const FString Seen=F->Director->PreviewPlace(TEXT("farm_lights")),Heard=F->Director->PreviewPlace(TEXT("town_lights")),Trace=F->Director->PreviewPlace(TEXT("landing_trace"));
  for(const FS47EventPlace& P:F->EventPlaces->All()){UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 place %s: %s, %s, %.0f m, last night %d, used %d"),*P.Id,*P.Title,*FS47EventPlaces::AreaOf(P),F->EventPlaces->DistanceFromPlayer(P),F->EventPlaces->LastNight(P.Id),F->EventPlaces->TimesUsed(P.Id));}
  Tell(true,FString::Printf(TEXT("Lights seen from here: %s. Heard of: %s. Marked ground: %s. %d places on the list."),Seen.IsEmpty()?TEXT("none"):*Name(Seen),Heard.IsEmpty()?TEXT("none"):*Name(Heard),Trace.IsEmpty()?TEXT("none"):*Name(Trace),F->EventPlaces->All().Num()),9);
 });
}

// ---- Reyes, Ward's review, and the keys ----
void FS47DevFreeform::GoReyes(const FString& Id,const FString& Say)
{
 auto* F=Session();if(!F||!F->Predecessor){Tell(false,TEXT("Freeform only"));return;}
 F->Predecessor->TestOn();
 FString Area;double X=0,Z=0;
 if(!F->Predecessor->WorldSpot(Id,Area,X,Z)){Tell(false,FString::Printf(TEXT("%s is not in this scene yet"),*Id));return;}
 FStage Stage;Stage.Area=Area;
 const bool bStation=Area==TEXT("saro");
 const double OffZ=bStation?0:3.5;
 Stage.Where=[X,Z,OffZ]{return FVector2D(X,Z+OffZ);};Stage.Yaw=bStation?UE_DOUBLE_HALF_PI:0;
 Go(Stage,[this,Say]{Tell(true,Say,6);});
}
void FS47DevFreeform::AddReyes(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Reyes: the trail");
 static const TCHAR* const W=TEXT("Ward: the review and how it ends");
 const auto Pred=[this]()->FS47Predecessor*{auto* F=Session();if(!F||!F->Predecessor){Tell(false,TEXT("Freeform only"));return nullptr;}F->Predecessor->TestOn();return F->Predecessor.Get();};
 E.AddFreeform(G,TEXT("Go: Reyes's locker"),TEXT("the east wing's storage room; E opens it"),[this]{GoReyes(TEXT("locker"),TEXT("Reyes's locker, in front of you. E opens it."));});
 E.AddFreeform(G,TEXT("Go: wagon 23"),TEXT("on the flat, on foot, as it stands; E on it, the radio is on"),[this]{GoReyes(TEXT("wagon"),TEXT("Wagon 23. E on it. Its radio is on, and comes back on the next day if you switch it off."));});
 for(const S47PredecessorText::FTagDef& D:S47PredecessorText::Tags())
 {
  const FString Doc=D.Doc,Place=D.Place;
  E.AddFreeform(G,FString::Printf(TEXT("Go: Reyes's tag at the %s"),*Place),TEXT("on foot, in front of it; E reads it, and it points on"),[this,Doc,Place]{GoReyes(Doc,FString::Printf(TEXT("Reyes's survey tag at the %s. E reads it."),*Place));});
 }
 E.AddFreeform(G,TEXT("Read: April's binder, and its gap"),TEXT("the page opens here; records room's binder or OPS archive in play"),[Pred]{if(auto* P=Pred())P->TestFound(TEXT("gap"));});
 for(const S47PredecessorText::FCaseNote& N:S47PredecessorText::CaseNotes())
 {
  const FString Doc=N.Doc,Case=N.CaseTitle;
  E.AddFreeform(G,FString::Printf(TEXT("Read: Reyes's note in %s"),*Case),TEXT("turns up in the tray in play, when that case opens"),[Pred,Doc]{if(auto* P=Pred())P->TestFound(Doc);});
 }
 for(const TCHAR* Who:{TEXT("dale"),TEXT("attendant"),TEXT("deputy"),TEXT("host"),TEXT("radio_woman"),TEXT("souvenir_man"),TEXT("ward")})
 {
  const FString Id=Who;
  E.AddFreeform(G,FString::Printf(TEXT("Talk: %s"),S47PredecessorText::Person(Id)->Title),TEXT("the words they have for the new operator"),[Pred,Id]{if(auto* P=Pred())P->Talk(Id);});
 }
 E.AddFreeform(G,TEXT("Dale on the walkie: the next line that is due"),TEXT("each once, at most one a day in play; gives you the set"),[Pred,this]{if(auto* P=Pred()){if(!P->TestBark())Tell(false,TEXT("Nothing is due from Dale yet: read the letter, open the locker, read a tag or file a job."));}});
 E.AddFreeform(G,TEXT("Phone: who to call"),TEXT("the same panel as the control room phone; Dale or Ward, and his slip if the walkie was not with you"),[Pred]{if(auto* P=Pred())P->UsePhone();});
 E.AddFreeform(G,TEXT("The phone rings: Nora"),TEXT("once, in play, after five finds; pick up the control room phone"),[Pred,this]{if(auto* P=Pred()){P->TestRing();Tell(true,TEXT("It rings for 45 seconds. The control room phone answers it."),5);}});
 E.AddFreeform(W,TEXT("Ward's first letter"),TEXT("in play it lies in the OPS tray on the first day; the date of a new game is day 1 = Monday April 28"),[Pred]{if(auto* P=Pred())P->TestLetter();});
 E.AddFreeform(W,TEXT("Ward's Monday note"),TEXT("as it would be this week, by what you have filed"),[Pred]{if(auto* P=Pred())P->TestWeek();});
 E.AddFreeform(W,TEXT("Run to the assessment"),TEXT("the clock jumps to the assessment day; the outcome is what this game's numbers give"),[Pred,this]{if(auto* P=Pred()){P->TestAssessNow();Tell(true,TEXT("Assessed by what is really there. The letter is in the OPS tray."),6);}});
 struct FResult{const TCHAR* Label;const TCHAR* Note;FS47Predecessor::EOutcome Outcome;};
 static const FResult Results[]={
  {TEXT("Outcome: SARO stays open"),TEXT("you play on; nothing removed"),FS47Predecessor::EOutcome::Stays},
  {TEXT("Outcome: closes, and you know what can be known"),TEXT("knowing, not solving; you can stay out the notice"),FS47Predecessor::EOutcome::KnowsEnough},
  {TEXT("Outcome: closes, and you have your originals"),TEXT("nothing removed; you can stay out the notice"),FS47Predecessor::EOutcome::ClosedOriginals},
  {TEXT("Outcome: closes, with nothing to show"),TEXT("you can stay out the notice"),FS47Predecessor::EOutcome::ClosedNothing},
  {TEXT("Outcome: you are gone, and someone else starts"),TEXT("the letter to the next operator; go back, or start as them"),FS47Predecessor::EOutcome::Gone},
 };
 for(const FResult& R:Results)
 {
  const FS47Predecessor::EOutcome Outcome=R.Outcome;
  E.AddFreeform(W,R.Label,R.Note,[Pred,Outcome]{if(auto* P=Pred())P->TestOutcome(Outcome);});
 }
 E.AddFreeform(W,TEXT("Go: the key board"),TEXT("by the lockers; E hangs up your keys, and asks first"),[this]{GoReyes(TEXT("keys"),TEXT("The key board beside the lockers. E, and it asks you first."));});
 E.AddFreeform(W,TEXT("Keys: the question"),TEXT("the same panel as at OPS and at the board"),[Pred]{if(auto* P=Pred())P->OpenKeys();});
 E.AddFreeform(W,TEXT("Keys: leave, as things stand"),TEXT("the ending, from this game's real state; take them back, or go to the title"),[Pred]{if(auto* P=Pred())P->TestLeave();});
}

void FS47DevFreeform::AddEntries(FS47DevEvents& E)
{
 Menu=&E;
 AddRunNight(E);
 AddTimePlace(E);
 AddDirector(E);
 AddIndoors(E);
 AddSites(E);
 AddHighway(E);
 AddTaken(E);
 AddAfter(E);
 AddLand(E);
 AddAnywhere(E);
 AddReyes(E);
}

void FS47DevFreeform::Stop()
{
 // A new game or the title: the session these were set on has gone.
 Waiting.Reset();bNatural=false;Speed=SpeedUsed=1;Holding.Empty();bFlagRead=false;
 if(Menu)
 {
  Menu->Relabel(TEXT("Natural rhythm: ON"),TEXT("Natural rhythm: off"));
  for(const TCHAR* L:SpeedLabels)Menu->Relabel(FString::Printf(TEXT("Speed: %s"),L),TEXT("Speed: x1"));
 }
 bPanelFull=false;
 if(Menu){Menu->Relabel(TEXT("Panel: short"),TEXT("Panel: off"));Menu->Relabel(TEXT("Panel: full"),TEXT("Panel: off"));}
 if(M&&M->Rhythm)M->Rhythm->SetWatching(false);
 ShowPanel(false);
}

void FS47DevFreeform::Step(double Dt)
{
 if(!M)return;
 // Ticks only while the night runs: a menu holds them as it holds the night.
 if(M->Mode==US47Main::EMode::Play&&!M->bPausedByMenu&&!M->ModalOpen()&&Dt>0)
 {
  // Run in turn, a finished one may add another.
  TArray<FWait> Now=MoveTemp(Waiting);Waiting.Reset();
  for(FWait& W:Now)
  {
   W.Left-=Dt;
   if(W.Cond)
   {
    if(W.Cond()){if(W.Fn)W.Fn();}
    else if(W.Left<=0){if(W.Else)W.Else();}
    else Waiting.Add(MoveTemp(W));
   }
   else if(W.Left<=0){if(W.Fn)W.Fn();}
   else Waiting.Add(MoveTemp(W));
  }
 }
 if(!Session())return;
 if(!bFlagRead)
 {
  bFlagRead=true;
  // -S47NaturalNight=15: a test run that starts this way, from the title's Developer screen or a script.
  double Flag=0;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47NaturalNight="),Flag)&&DevRun())
  {
   Speed=FMath::Clamp(Flag,1.,60.);SetNatural(true);
   if(Menu)Menu->Relabel(TEXT("Natural rhythm: off"),TEXT("Natural rhythm: ON"));
   // -S47NaturalDay=2: begin at 17:00 on that day, past the first night's grace, with every rule live. Nothing else is set.
   int32 FromDay=0;
   if(auto* F0=Session();F0&&FParse::Value(FCommandLine::Get(),TEXT("S47NaturalDay="),FromDay)&&FromDay>F0->Day())
   {
    const double To=(FromDay-1)*86400.+17*3600.;
    F0->TestAdvance(FMath::Max(0.,To-F0->Clock()));
    UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 natural night from day %d %s"),F0->Day(),*ClockText(F0->Clock()));
   }
   FParse::Value(FCommandLine::Get(),TEXT("S47NaturalStand="),StandPlace);
  }
 }
 if(bNatural){StepNatural(Dt);StepStand();}
 StepSweep(Dt);
}

// Every line of the menu's Freeform groups, by its own function, from where the last one left off. Reads what each says about itself; how
// it looks is for the pictures, and a line that started may still be wrong to the eye.
void FS47DevFreeform::StepSweep(double Dt)
{
 if(!bSweepInit)
 {
  bSweepInit=true;FParse::Value(FCommandLine::Get(),TEXT("S47DevSweep="),SweepDir);FParse::Value(FCommandLine::Get(),TEXT("S47DevSweepGroup="),SweepOnly);
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!SweepDir.IsEmpty()&&(!SweepDir.StartsWith(TEXT("/tmp/"))||!User.StartsWith(TEXT("/tmp/"))||!DevRun())){UE_LOG(LogS47DevFreeform,Error,TEXT("S47: F10 sweep needs a /tmp directory and profile, in a developer run"));SweepDir.Empty();FPlatformMisc::RequestExitWithStatus(false,1);}
 }
 if(SweepDir.IsEmpty()||!Menu)return;
 TArray<const FS47DevEvents::FEvent*> Lines;
 for(const auto& Ev:Menu->Events())if(Ev.bFreeform&&Ev.Group!=TEXT("Run the night")&&Ev.Group!=TEXT("Time and place")&&!Ev.Group.StartsWith(TEXT("No line"))&&(SweepOnly.IsEmpty()||Ev.Group==SweepOnly))Lines.Add(&Ev);
 SweepWait-=Dt;
 if(bSweepPending)
 {
  // The line's own work, then a moment for it to show.
  if(!Waiting.IsEmpty()&&SweepWait>-75)return;
  if(SweepWait>0)return;
  const auto* L=Lines[SweepIndex];
  (bLastOk?SweepGood:SweepBad)+=1;
  UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 sweep %d/%d %s: %s / %s: %s"),SweepIndex+1,Lines.Num(),bLastOk?TEXT("PASS"):TEXT("FAIL"),*L->Group,*L->Label,*LastText);
  FScreenshotRequest::RequestScreenshot(SweepDir/FString::Printf(TEXT("sweep-%03d.png"),SweepIndex),false,false);
  bSweepPending=false;++SweepIndex;SweepWait=.5;
  return;
 }
 if(SweepWait>0)return;
 if(SweepIndex>=Lines.Num())
 {
  UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 sweep COMPLETE: %d started, %d refused, of %d"),SweepGood,SweepBad,Lines.Num());
  SweepDir.Empty();FPlatformMisc::RequestExitWithStatus(false,SweepBad>0?1:0);return;
 }
 ClearRunning();
 if(M->UI)M->UI->Close(true);
 bLastOk=true;LastText=TEXT("no word from the line");
 const auto* L=Lines[SweepIndex];
 UE_LOG(LogS47DevFreeform,Display,TEXT("S47: F10 sweep line %d/%d: %s / %s"),SweepIndex+1,Lines.Num(),*L->Group,*L->Label);
 if(L->Fn)L->Fn();
 bSweepPending=true;SweepWait=4;
}
