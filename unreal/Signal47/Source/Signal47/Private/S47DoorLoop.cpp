#include "S47DoorLoop.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47CalicheAccess.h"
#include "S47DevEvents.h"
#include "S47FarmHouse.h"
#include "S47FarmSite.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47NightTiers.h"
#include "S47OutsideLight.h"
#include "S47SaroSave.h"
#include "S47Script.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DoorLoop,Log,All);

// N1110 (N1110-6, N1110-7): see S47DoorLoop.h.

namespace S47DoorLoopLocal
{
 // The two outer doors of the SARO building, as walks.saro has their openings (FS47SC04's gaps): the leaf's plane on x, the
 // doorway across z, and which way is out.
 struct FDoor { const TCHAR* Id; double Plane, Z0, Z1, Out; };
 const FDoor Doors[2]={{TEXT("exitDoor"),-6.27,5.25,6.15,-1.},{TEXT("doorEast"),6.06,1.12,2.08,1.}};
 // The south corridor, between the records door and the control room door: where the ceiling is.
 constexpr double CeilX=-3.9,CeilZ=5.75;
 constexpr double FallFrom=.95,FallFromCalm=.55,Hang=.22,Gravity=9.8;
 constexpr uint32 Salt=0x444c4f4fu;
 FS47FreeformSession* Session(US47Main* M){return M?M->Freeform.Get():nullptr;}
 FString Key(int32 Day){return FString::Printf(TEXT("door-loop-d%d"),Day);}
 // N1110-7
 // N1110 (pace, critic 2): a taking (the record taken-<day>-<clock>) or the broken rule has been kept on this day: the night's one big thing is done.
 bool BigThingKey(const FString& Key,int32 Day,const FS47FreeformSession* F){return Key.StartsWith(FString::Printf(TEXT("taken-%d-"),Day))||(Key==TEXT("rule-pale-broken")&&F&&F->FindingDay(Key)==Day);}
 bool BigThingToday(const FS47FreeformSession& F)
 {
  for(const FString& K:F.FindingIdsFrom(F.Day()))if(BigThingKey(K,F.Day(),&F))return true;
  return false;
 }
 FString ChaseKey(int32 Day){return FString::Printf(TEXT("door-loop-chase-d%d"),Day);}
 const FVector HouseFrame(-24,0,300); // The old house stands in the farm's part of the SARO area (as FS47FarmHouseDoors has it).
 bool Outer(const FString& Name){return Name.StartsWith(TEXT("north"))||Name.StartsWith(TEXT("south"));}
 FString HouseId(const FString& Name){return TEXT("ff:olddoor:")+Name;}
 FString RoomId(const FString& Room){return TEXT("access:")+Room;}
 // N1110 (pace): after the three knocks, so that a door is tried. N1110 (pace, critic 2): three variants, rotated by the day.
 const TCHAR* const FrontDoors[3]={TEXT("Someone's at the front door."),TEXT("That was the front door. Knocking."),TEXT("Somebody is knocking. At this hour.")};
 constexpr double ThoughtNear=15.; // the thought is for one close enough to have heard the knocks at the front door, in metres
 const TCHAR* const AtTheDoor=TEXT("Something is at this door. Go out another way.");
 // The light at the windows: white, amber, blue, green.
 const FLinearColor FloodColors[4]={FLinearColor(.92f,.94f,1.f),FLinearColor(1.f,.6f,.18f),FLinearColor(.32f,.48f,1.f),FLinearColor(.38f,1.f,.48f)};
 constexpr double StrobeAfter=1.5,StrobeEvery=4.6,StrobeLong=1.,StrobeRate=2.5; // N1110 critic: at most three flashes a second for everyone (photosensitivity); Calm takes the bursts away
 FString Upper(FString S){if(!S.IsEmpty())S[0]=FChar::ToUpper(S[0]);return S;}
}

// ---------- the rule ----------

FS47DoorLoop::EStep FS47DoorLoop::FMachine::Press()
{
 if(Stage==EStage::Armed)Stage=EStage::Running;
 if(Stage!=EStage::Running)return EStep::Normal;
 Uses=FMath::Min(Uses+1,Cap);
 const EStep S=StepFor(Uses);
 if(S==EStep::Out)Stage=EStage::Done;
 return S;
}

FS47DoorLoop::EStep FS47DoorLoop::StepFor(int32 N)
{
 if(N<=0)return EStep::Normal;
 if(N<=SideLoops)return EStep::Side;
 if(N==SideLoops+1)return EStep::Ceiling;
 if(N==SideLoops+2)return EStep::Floor;
 return EStep::Out; // the cap: the sixth use always goes out
}

double FS47DoorLoop::LightAt(int32 N){return N<=0||N>=Cap?1.:FMath::Max(.3,1.-.14*N);}

int32 FS47DoorLoop::JumpMinutes(int32 N){return N>=1&&N<=SideLoops?N:0;}

int32 FS47DoorLoop::MissingMinutes(int32 Seed,int32 Night)
{
 return FMath::Clamp(10+FMath::RoundToInt(15.f*FS47NightRhythm::Roll(Seed,Night,S47DoorLoopLocal::Salt)),10,25);
}

int32 FS47DoorLoop::FinalMinutes(int32 Seed,int32 Night)
{
 int32 Spent=0;for(int32 U=1;U<Cap;++U)Spent+=JumpMinutes(U);
 return FMath::Max(1,MissingMinutes(Seed,Night)-Spent);
}

const TCHAR* FS47DoorLoop::ThoughtFor(int32 N,bool bFromFront)
{
 switch(N)
 {
 case 1:return bFromFront?TEXT("That was the front door."):TEXT("That was the service door.");
 case 2:return TEXT("No.");
 case 3:return TEXT("I went out. I know I went out.");
 default:return TEXT("");
 }
}

FString FS47DoorLoop::RecordText(int32 Minutes,bool bFromFront)
{
 return FString::Printf(TEXT("SARO, in the small hours. I went out the %s and came in at the %s. Then again, the other way, and again. The lights were lower each time. Once I was up by the ceiling of the corridor and fell, and once I came up through the floor. When a door finally let me out, the clock was about %d minutes ahead of where it should have been."),
  bFromFront?TEXT("front door"):TEXT("service door"),bFromFront?TEXT("service door"):TEXT("front door"),Minutes);
}

// N1110-7: the chase's light. Never the ordinary taking's numbers: stronger, and its own colours.
double FS47DoorLoop::FloodK(double Seconds,bool bCalm)
{
 using namespace S47DoorLoopLocal;
 const double T=FMath::Max(0.,Seconds);
 // Calm flashes: it comes up over a second and a half and only breathes, slowly; no strobe, nothing sudden.
 if(bCalm)return FMath::Clamp(T/1.5,0.,1.)*(1.2+.1*FMath::Sin(T*1.1));
 const double Base=FMath::Clamp(T/.25,0.,1.)*(1.3+.12*FMath::Sin(T*2.3));
 if(T<StrobeAfter)return Base;
 const double Into=FMath::Fmod(T-StrobeAfter,StrobeEvery);
 if(Into<StrobeLong)return FMath::Fmod(Into*StrobeRate,1.)<.5?1.7:.08;
 return Base;
}

FLinearColor FS47DoorLoop::FloodColor(double Seconds,bool bCalm)
{
 using namespace S47DoorLoopLocal;
 const double T=FMath::Max(0.,Seconds);
 // In a burst, each flash its own colour.
 if(!bCalm&&T>=StrobeAfter)
 {
  const double Into=FMath::Fmod(T-StrobeAfter,StrobeEvery);
  if(Into<StrobeLong)return FloodColors[FMath::Clamp(FMath::FloorToInt(Into*StrobeRate),0,1000)%4];
 }
 // Between: one colour, then a quick turn to the next (calm: a slow fade the whole way).
 const double Span=bCalm?3.2:1.6,Blend=bCalm?1.:.25;
 const double U=FMath::Fmod(T/Span,4.);
 const int32 A=FMath::Clamp(FMath::FloorToInt(U),0,3);
 const double K=FMath::Clamp((U-A-(1.-Blend))/Blend,0.,1.);
 return FLinearColor::LerpUsingHSV(FloodColors[A],FloodColors[(A+1)%4],float(K*K*(3.-2.*K)));
}

double FS47DoorLoop::ShadeK(double Seconds,bool bCalm)
{
 const double X=FMath::Clamp(Seconds/ShadeFor(bCalm),0.,1.);
 return 1.-(bCalm?.55:.92)*FMath::Square(FMath::Sin(UE_DOUBLE_PI*X));
}

const TCHAR* FS47DoorLoop::ChaseThought(int32 N)
{
 switch(N)
 {
 case 1:return TEXT("That was the front door. I'm back inside.");
 case 2:return TEXT("Something went past the window.");
 case 3:return TEXT("Somebody is trying the handle.");
 default:return TEXT("");
 }
}

FString FS47DoorLoop::ChaseRecord(const FString& Title,int32 Minutes)
{
 return FString::Printf(TEXT("%s, in the small hours. Something was outside, and I tried to get away through a door. Every door I went out of brought me back inside. The light came in at every window, white, then amber, blue and green, and something walked round the walls and tried the handle. Once I was up by the ceiling and fell, and once I came up through the floor. When a door finally let me out there was nothing there, and the clock was about %d minutes ahead of where it should have been."),
  *S47DoorLoopLocal::Upper(Title.IsEmpty()?FString(TEXT("the building")):Title),Minutes);
}

// ---------- the building ----------

FS47DoorLoop::FPlace FS47DoorLoop::SaroPlace()
{
 using namespace S47DoorLoopLocal;
 FPlace P;P.Kind=EPlace::Saro;P.Area=TEXT("saro");P.Title=TEXT("SARO");
 for(const FDoor& D:Doors)
 {
  FGate G;G.Id=D.Id;G.Mid=FVector(D.Plane,0,.5*(D.Z0+D.Z1));G.Out=FVector(D.Out,0,0);G.Half=.5*(D.Z1-D.Z0);
  P.Gates.Add(G);
 }
 P.Up=FVector(CeilX,0,CeilZ);P.UpYaw=-UE_DOUBLE_HALF_PI;P.bUp=P.bUpYaw=true;
 return P;
}

double FS47DoorLoop::SideOf(const FGate& G,double X,double Z){return (X-G.Mid.X)*G.Out.X+(Z-G.Mid.Z)*G.Out.Z;}

bool FS47DoorLoop::InBand(const FGate& G,double X,double Z,double Pad){return FMath::Abs((Z-G.Mid.Z)*G.Out.X-(X-G.Mid.X)*G.Out.Z)<=G.Half+Pad;}

// The walker faces (-sin yaw, -cos yaw): facing in, away from the door, is facing -Out.
double FS47DoorLoop::YawIn(const FGate& G){return FMath::Atan2(G.Out.X,G.Out.Z);}

bool FS47DoorLoop::PlaceAt(FPlace& Out) const
{
 using namespace S47DoorLoopLocal;
 if(!M||!M->Player||!M->Web||!M->World||M->World->bDriving)return false;
 if(InStation()){Out=SaroPlace();return true;}
 const FVector& P=M->Player->Pos;
 const FString Area=M->Web->Area();
 const FS47FreeformSession* F=Session(M);
 if(Area==TEXT("saro")&&F&&F->HouseDoors&&S47FarmHouse::Inside(P.X-HouseFrame.X,P.Z-HouseFrame.Z))
 {
  FPlace H;H.Kind=EPlace::House;H.Area=Area;H.Room=TEXT("oldhouse");H.Title=TEXT("the old house");
  const FVector Hall=S47FarmHouse::Point(6,4)+HouseFrame;
  for(int32 I=0;I<F->HouseDoors->Num();++I)
  {
   const FString Name=F->HouseDoors->NameOf(I);
   if(!Outer(Name))continue;
   FGate G;G.Id=HouseId(Name);G.Index=I;G.Mid=F->HouseDoors->Middle(I);G.Mid.Y=0; // heights are the player's floor's
   FVector N=F->HouseDoors->Normal(I);N.Y=0;N=N.GetSafeNormal();
   if((G.Mid.X-Hall.X)*N.X+(G.Mid.Z-Hall.Z)*N.Z<0)N=-N;
   G.Out=N;H.Gates.Add(G);
  }
  H.Up=Hall;H.bUp=true;
  if(!H.Gates.IsEmpty()){Out=H;return true;}
 }
 if(Area==TEXT("road")||Area==TEXT("diner"))
 {
  FString Id;FVector Mid,N,Middle;
  if(S47CalicheAccess::RoomAt(P,Id)&&S47CalicheAccess::DoorAt(Id,Mid,N)&&S47CalicheAccess::RoomMiddle(Id,Middle))
  {
   FPlace R;R.Kind=EPlace::Room;R.Area=Area;R.Room=Id;R.Title=TEXT("the room in town");
   N.Y=0;N=N.GetSafeNormal();
   if((Mid.X-Middle.X)*N.X+(Mid.Z-Middle.Z)*N.Z<0)N=-N;
   FGate G;G.Id=RoomId(Id);G.Mid=FVector(Mid.X,0,Mid.Z);G.Out=N;R.Gates.Add(G);
   R.Up=Middle;R.bUp=true;
   Out=R;return true;
  }
 }
 return false;
}

bool FS47DoorLoop::InPlace() const
{
 if(Where.Kind==EPlace::Saro)return InStation();
 FPlace Now;
 return PlaceAt(Now)&&Now.Same(Where);
}

bool FS47DoorLoop::ThereStill() const
{
 return M&&M->Web&&M->World&&M->Player&&M->Web->Area()==Where.Area&&(Where.Kind!=EPlace::Saro||M->World->Area==Where.Area)&&!M->World->bDriving;
}

bool FS47DoorLoop::GateOpen(const FPlace& P,int32 I) const
{
 if(!M||!P.Gates.IsValidIndex(I))return false;
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 switch(P.Kind)
 {
 case EPlace::Saro:return M->Web&&M->Web->IsDoorOpen(P.Gates[I].Id);
 case EPlace::House:return F&&F->HouseDoors&&F->HouseDoors->IsOpen(P.Gates[I].Index);
 case EPlace::Room:return S47CalicheAccess::IsOpen(P.Room);
 default:return false;
 }
}

void FS47DoorLoop::GateShut(const FPlace& P,int32 I)
{
 if(!M||!P.Gates.IsValidIndex(I))return;
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(P.Kind==EPlace::Saro&&M->Web)M->Web->SetDoorOpen(P.Gates[I].Id,false,true);
 else if(P.Kind==EPlace::House&&F&&F->HouseDoors)F->HouseDoors->SetOpenQuietly(P.Gates[I].Index,false);
 else if(P.Kind==EPlace::Room){S47CalicheAccess::SetOpenQuietly(P.Room,false);S47CalicheAccess::PoseDoor(P.Room,-1);}
}

void FS47DoorLoop::GateSwingShut(const FPlace& P,int32 I)
{
 if(!M||!P.Gates.IsValidIndex(I))return;
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(P.Kind==EPlace::Saro&&M->Web)
 {
  M->Web->SetDoorOpen(P.Gates[I].Id,true,true);
  M->Web->SetDoorOpen(P.Gates[I].Id,false,false);
  return;
 }
 // The old house's leaf swings shut from part open; a room's is shut at once. Each with the closing sound.
 if(P.Kind==EPlace::House&&F&&F->HouseDoors)
 {
  const int32 Index=P.Gates[I].Index;
  if(F->HouseDoors->IsOpen(Index))F->HouseDoors->SetOpenQuietly(Index,false);
  F->HouseDoors->PoseOver(Index,.6);F->HouseDoors->PoseOver(Index,-1);
 }
 else if(P.Kind==EPlace::Room){S47CalicheAccess::SetOpenQuietly(P.Room,false);S47CalicheAccess::PoseDoor(P.Room,-1);}
 if(M->Audio)M->Audio->Sfx(TEXT("doorClose"),FS47Sound{.Gain=.7f,.At=P.Gates[I].Mid+FVector(0,M->Player?M->Player->FloorY+1:1,0)});
}

void FS47DoorLoop::GateKnob(const FPlace& P,int32 I,double K)
{
 if(!M||!P.Gates.IsValidIndex(I))return;
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(P.Kind==EPlace::House&&F&&F->HouseDoors)F->HouseDoors->PoseOver(P.Gates[I].Index,K);
 else if(P.Kind==EPlace::Room)S47CalicheAccess::PoseDoor(P.Room,K);
 // SARO's doors are the scene's own: the latch is heard, the leaf does not move.
}

// ---------- the session's ----------

FS47DoorLoop::~FS47DoorLoop(){}

void FS47DoorLoop::Begin()
{
 // Nothing of a loop is kept: a Continue or a load mid-loop is an ordinary station (Stop has put the lights and the eye back).
 Restore();
 Run.Reset();Source.Reset();LastDay=0;BigNight=0;PendingFrom=-1;PendingStep=EStep::Normal;From=0;
 ArmedUntil=Idle=BigAfter=0;Pending=ThoughtIn=RadioOff=-1;ThoughtText.Reset();WasIn.Reset();bBigSeen=false;
 Where=FPlace();Watch=FPlace();ChaseDay=ChaseNight=0;DevChaseLeft=0;
}

void FS47DoorLoop::Stop()
{
 Restore();
 Run.Reset();Pending=-1;PendingFrom=-1;ThoughtIn=-1;
 Watch=FPlace();DevChaseLeft=0;
 if(Flood)Flood->Stop();
 Flood.Reset();
}

void FS47DoorLoop::Restore()
{
 // N1110-7: the light at the windows out, the air clear, a handle let go, nothing more walking round outside.
 if(Flood){Flood->Hide();Flood->SetAir(0,FVector::ZeroVector,FLinearColor::White,true);}
 if(KnobGate>=0)GateKnob(Where,KnobGate,-1);
 KnobGate=-1;KnobT=-1;KnobWait=0;StepsLeft=0;ShadeT=-1;FloodT=0;FloodFind=0;bFloodWay=false;bWarned=false;bChase=false;
 if(bDimmed&&M&&M->Web)M->Web->SetLightsScale(1.f,1.f);
 bDimmed=false;
 if(bRadioOurs&&M&&M->Audio)M->Audio->AmRadioThree(TOptional<FVector>());
 bRadioOurs=false;RadioOff=-1;
 if((bFalling||RiseT>=0)&&M&&M->Player&&!M->Player->InAir())M->Player->Lift=0;
 bFalling=false;FallHang=-1;FallV=0;RiseT=-1;
}

bool FS47DoorLoop::InStation() const
{
 return M&&M->Web&&M->World&&M->Player&&M->Web->Area()==TEXT("saro")&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&S47SaroSave::InStation(M);
}

int32 FS47DoorLoop::LastLoopDay() const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 int32 Last=LastDay;
 if(F)for(int32 D=F->Day();D>FMath::Max(0,F->Day()-WeekDays);--D)if(F->HasFinding(S47DoorLoopLocal::Key(D))){Last=FMath::Max(Last,D);break;}
 return Last;
}

bool FS47DoorLoop::Gates(FString& Why,bool bDev) const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F||!F->Active()||F->Dead()){Why=TEXT("Freeform only");return false;}
 if(!InStation()){Why=TEXT("inside the SARO building, on foot");return false;}
 if(F->Taken&&F->Taken->Busy()){Why=TEXT("being taken");return false;}
 if(F->SkyCutLive()||!F->HasPowerForReactiveLight()){Why=TEXT("the power is not steady");return false;}
 if(bDev)return true;
 if(!F->RulesLive()){Why=TEXT("not a live night");return false;}
 if(F->InTutorial()){Why=TEXT("the first shift");return false;}
 if(!MayLoop(F->Day(),LastLoopDay())){Why=TEXT("once a week, not before the fourth night");return false;}
 const double Clock=F->Clock(),Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 if(!S47NightTiers::Allows(ES47Allowed::Full,Hour,FS47FreeformSession::NightOf(Clock))||Hour>=LastHour){Why=TEXT("the full tier only, to half past five");return false;}
 if(F->Director&&(F->Director->DangerTonight()>0||F->Director->Busy())){Why=TEXT("one danger a night");return false;}
 if(S47DoorLoopLocal::BigThingToday(*F)){Why=TEXT("one big thing a night: a taking or a broken rule has played");return false;} // N1110 (pace, critic 2)
 if(F->Happenings&&F->Happenings->BigLive()){Why=TEXT("the big night is under way");return false;}
 return true;
}

bool FS47DoorLoop::Arm(const TCHAR* Why,bool bDev)
{
 FString Not;
 if(Run.Live()||!Gates(Not,bDev))return false;
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F||!Run.Arm())return false;
 Source=Why;ArmedUntil=F->Clock()+ArmedFor;Idle=0;
 Where=SaroPlace();bChase=false;
 WasIn.Init(false,Where.Gates.Num());
 for(int32 I=0;I<WasIn.Num();++I)WasIn[I]=InsideOf(I);
 // Quiet first: the night outside stops all at once, then three soft knocks at the front door.
 if(M->Audio&&M->Audio->Night.IsValid())M->Audio->Night->Hush(12,.04,.12,3.5);
 const FS47WebItem* DoorIt=M->Web->Item(S47DoorLoopLocal::Doors[0].Id);
 const FVector DoorAt=DoorIt?M->Web->ObjAt(DoorIt->Node).WorldPosition():FVector(S47DoorLoopLocal::Doors[0].Plane,1.2,5.7);
 if(M->Audio)
 {
  const FVector At=DoorAt;
  for(int32 K=0;K<3;++K)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.55f,.Rate=.9f,.At=At,.When=4.f+.5f*K});
 }
 // N1110 (pace): the knocks end at 5 s; the thought follows, so the player is drawn to try a door (not from the far end of the building)
 if(M->Player&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(DoorAt.X,DoorAt.Z))<=S47DoorLoopLocal::ThoughtNear)Think(S47DoorLoopLocal::FrontDoors[FMath::Abs(F->Day())%3],5.6);
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop armed (%s), day %d"),Why,F->Day());
 return true;
}

bool FS47DoorLoop::TakeVisit()
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 FString Why;
 if(!F||Run.Live()||!Gates(Why,false))return false;
 if(FS47NightRhythm::Roll(F->Seed(),FS47FreeformSession::NightOf(F->Clock()),S47DoorLoopLocal::Salt+1)>=InsteadOdds)return false;
 return Arm(TEXT("attention"),false);
}

int32 FS47DoorLoop::DoorOf(const FString& Id) const
{
 for(int32 I=0;I<Where.Gates.Num();++I)if(Id==Where.Gates[I].Id)return I;
 return -1;
}

bool FS47DoorLoop::InsideOf(int32 Door) const
{
 if(!M||!M->Player||!Where.Gates.IsValidIndex(Door))return false;
 return SideOf(Where.Gates[Door],M->Player->Pos.X,M->Player->Pos.Z)<0;
}

bool FS47DoorLoop::Arrival(int32 Door,double& X,double& Z,double& Yaw) const
{
 if(!M||!M->Player||!Where.Gates.IsValidIndex(Door))return false;
 const FGate& G=Where.Gates[Door];
 Yaw=YawIn(G); // facing in, away from the door, as if just come in
 for(const double In:{1.6,1.3,1.,.75})
 {
  X=G.Mid.X-G.Out.X*In;Z=G.Mid.Z-G.Out.Z*In;
  if(M->Player->Walkable(X,Z))return true;
 }
 return false;
}

bool FS47DoorLoop::Use(const FString& Id)
{
 if(!M||!M->Web)return false;
 if(!Run.Live())
 {
  // N1110-7: not running, but something is after the player and this is a way out of the building they are in.
  if(Id!=S47DoorLoopLocal::Doors[0].Id&&Id!=S47DoorLoopLocal::Doors[1].Id&&!Id.StartsWith(TEXT("ff:olddoor:"))&&!Id.StartsWith(TEXT("access:")))return false;
  FPlace At;
  if(!PlaceAt(At))return false;
  int32 Gate=-1;
  for(int32 I=0;I<At.Gates.Num();++I)if(At.Gates[I].Id==Id)Gate=I;
  if(Gate<0||!M->Player||SideOf(At.Gates[Gate],M->Player->Pos.X,M->Player->Pos.Z)>=0||GateOpen(At,Gate))return false;
  if(!StartChase(At))return false;
 }
 const int32 Door=DoorOf(Id);
 if(Door<0)return false;
 if(Pending>=0)return true; // the cut is coming: nothing more of this door
 if(!InsideOf(Door)||GateOpen(Where,Door))return false; // from outside, or shutting it: an ordinary door
 if(Run.Stage==EStage::Armed)Start();
 if(Run.Uses==0)From=Door;
 const EStep S=Run.Press();
 if(S==EStep::Out){Break(Door);return false;} // it opens, as a door does
 // The latch goes; the door does not move. A moment later the player is somewhere else.
 const FS47WebItem* It=Where.Kind==EPlace::Saro?M->Web->Item(Id):nullptr;
 const TOptional<FVector> LatchAt=It?TOptional<FVector>(M->Web->ObjAt(It->Node).WorldPosition()):Where.Kind!=EPlace::Saro&&M->Player?TOptional<FVector>(Where.Gates[Door].Mid+FVector(0,M->Player->FloorY+1,0)):TOptional<FVector>();
 if(M->Audio)M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound{.Gain=.5f,.At=LatchAt});
 Pending=CutAfter;PendingStep=S;PendingFrom=Door;
 return true;
}

void FS47DoorLoop::Start()
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F)return;
 Idle=0;
 if(bChase)
 {
  // N1110-7: the hunt's own meeting, already counted: what was after them gives way, and is outside now.
  ChaseDay=F->Day();
  YieldHunt();
  FloodT=0;FloodFind=0;bFloodWay=false;ShadeT=-1;
  if(M->Audio&&M->Audio->Night.IsValid())M->Audio->Night->Hush(12,.04,.12,3.5);
 }
 else
 {
  LastDay=F->Day();
  if(F->Director&&Source!=TEXT("developer"))F->Director->NoteDanger(); // the night's one danger
 }
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop begins (%s) at %.2f %.2f"),*Source,M->Player?M->Player->Pos.X:0.,M->Player?M->Player->Pos.Z:0.);
}

void FS47DoorLoop::Cut(EStep S,int32 FromDoor)
{
 using namespace S47DoorLoopLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||!M->Player||!M->Web){Finish(TEXT("no world"));return;}
 const int32 Other=OtherOf(FromDoor),N=Run.Uses;
 double X=0,Z=0,Yaw=0;
 bool bSpot=Arrival(Other,X,Z,Yaw);
 if(S==EStep::Ceiling&&Where.bUp&&M->Player->Walkable(Where.Up.X,Where.Up.Z)){X=Where.Up.X;Z=Where.Up.Z;if(Where.bUpYaw)Yaw=Where.UpYaw;bSpot=true;}
 if(!bSpot){Finish(TEXT("nowhere to stand"));return;} // never stuck: an ordinary station at once
 M->Player->Place(X,Z,Yaw);
 // The other door has just let the player in (the same one, in a room with only one): it swings shut behind them, with its own sound.
 if(S==EStep::Side)GateSwingShut(Where,Other);
 else if(S==EStep::Ceiling)
 {
  bFalling=true;FallHang=Hang;FallV=0;M->Player->Lift=M->Player->bHeadMotion?FallFrom:FallFromCalm;
 }
 else if(S==EStep::Floor)
 {
  RiseSecs=M->Player->bHeadMotion?1.2:1.8;RiseDepth=1.75;RiseT=0;M->Player->Lift=-RiseDepth;
  if(M->Audio)M->Audio->Beep(46.f,float(RiseSecs)+.6f,.06f,0.f,TEXT("sine"));
 }
 // A step dimmer, the clock on, the night still holding its breath. In the chase the light outside has the room's lamps already.
 if(!bChase){M->Web->SetLightsScale(float(LightAt(N)),float(FMath::Max(.5,LightAt(N))));bDimmed=true;}
 if(JumpMinutes(N)>0)F->SpendTime(60.*JumpMinutes(N));
 if(M->Audio&&M->Audio->Night.IsValid())M->Audio->Night->Hush(8,.03,.12,3.5);
 if(bChase)
 {
  // N1110-7: what marks each step is outside: steps along the wall, a shape across the light, the handle of the door behind them.
  if(N==1)Footsteps(Other,6);
  if(N==2)
  {
   ShadeT=0;
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.6f,.Rate=.6f,.At=Where.Gates[Other].Mid+Where.Gates[Other].Out*2.5+FVector(0,M->Player->FloorY+1,0)});
  }
  if(N==3)Knob(Other,1.6,1.1);
  FloodFind=0; // from where they are now, it finds its way in again
  bWarned=false;
 }
 else if(N==2&&M->Audio&&!M->Audio->RadioOn())
 {
  const FS47Obj Set=M->Web->Obj(TEXT("room.objs.radio"));
  if(Set){M->Audio->AmRadioThree(Set.WorldPosition());bRadioOurs=true;RadioOff=4.5;}
 }
 if(N==3&&M->Audio&&!bChase) // (the chase has the handle and the light at this step: no phone over them)
 {
  const FS47WebItem* Phone=M->Web->Item(TEXT("phone"));
  FS47Sound Ring;Ring.Gain=.45f;Ring.When=1.2f;Ring.Dur=1.1f;
  if(Phone)Ring.At=M->Web->ObjAt(Phone->Node).WorldPosition();
  M->Audio->Sfx(TEXT("phoneRing"),Ring);
 }
 const FString Said=bChase?FString(ChaseThought(N)):FString(ThoughtFor(N,From==0));
 if(!Said.IsEmpty())Think(Said,.9);
 Idle=0;for(bool& B:WasIn)B=false; // a cut is never a walk out
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop %d/%d: step %d, from %s, now at %.2f %.2f%s"),N,Cap,int32(S),Where.Gates.IsValidIndex(FromDoor)?*Where.Gates[FromDoor].Id:TEXT("-"),X,Z,bChase?TEXT(" (chase)"):TEXT(""));
}

void FS47DoorLoop::Break(int32 FromDoor)
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 const bool bWasChase=bChase;
 const FString Title=ChaseTitle;
 Restore();
 if(F)
 {
  const int32 Night=FS47FreeformSession::NightOf(F->Clock());
  F->SpendTime(60.*FinalMinutes(F->Seed(),Night));
  const FString Rec=bWasChase?S47DoorLoopLocal::ChaseKey(F->Day()):S47DoorLoopLocal::Key(F->Day());
  if(!F->HasFinding(Rec))F->RecordEvidence(Rec,bWasChase?ChaseRecord(Title,MissingMinutes(F->Seed(),Night)):RecordText(MissingMinutes(F->Seed(),Night),From==0));
  if(F->Director)F->Director->RestFor(RestAfter); // relief: quiet minutes after it
  if(bWasChase)ChaseDay=F->Day();else LastDay=F->Day();
 }
 // N1110-7: the light gone and nothing at the windows: relief, and the time it cost.
 Think(bWasChase?TEXT("It's gone. How long was I in there?"):TEXT("How long was I in there?"),2.2);
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop broken at the %s after %d uses%s"),Where.Gates.IsValidIndex(FromDoor)?*Where.Gates[FromDoor].Id:TEXT("way out"),Run.Uses,bWasChase?TEXT(" (chase: it is gone)"):TEXT(""));
 Run.Reset();Pending=-1;PendingFrom=-1;
 if(M)M->RequestAutosave();
}

void FS47DoorLoop::Finish(const TCHAR* Why)
{
 const bool bWas=Run.Live();
 Restore();
 Run.Reset();Pending=-1;PendingFrom=-1;
 if(bWas)UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop over: %s"),Why);
}

void FS47DoorLoop::StepMotion(double Dt)
{
 AS47Walker* P=M?M->Player.Get():nullptr;
 if(!P)return;
 // A jump of the player's own takes the eye over: the walker's arc lands it.
 if(P->InAir()){bFalling=false;RiseT=-1;return;}
 if(bFalling)
 {
  if(FallHang>0){FallHang-=Dt;return;}
  FallV-=S47DoorLoopLocal::Gravity*Dt;P->Lift=FMath::Max(0.,P->Lift+FallV*Dt);
  if(P->Lift<=0)
  {
   bFalling=false;P->Lift=0;
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.9f,.Rate=.8f,.At=FVector(P->Pos.X,P->FloorY+.1,P->Pos.Z)});
   if(P->bHeadMotion)P->Shake=FMath::Max(P->Shake,.35);
   if(P->OnStep)P->OnStep();
  }
 }
 else if(RiseT>=0)
 {
  RiseT+=Dt;
  const double K=FMath::Clamp(RiseT/RiseSecs,0.,1.);
  P->Lift=-RiseDepth*(1.-K*K*(3.-2.*K));
  if(K>=1){P->Lift=0;RiseT=-1;}
 }
}

int32 FS47DoorLoop::Crossing(const FPlace& P,bool& bAtDoor)
{
 bAtDoor=false;
 if(!M||!M->Player)return -1;
 if(WasIn.Num()!=P.Gates.Num())WasIn.Init(false,P.Gates.Num());
 // Walked out through a door standing open: the same as E on it. WasIn: last seen clearly on the inside of that door's plane (in
 // the old house and a room, also in front of the doorway: their walls have corners a walk round outside could cross the plane at).
 const FVector& Pos=M->Player->Pos;
 for(int32 Door=0;Door<P.Gates.Num();++Door)
 {
  const FGate& G=P.Gates[Door];
  const double Side=SideOf(G,Pos.X,Pos.Z);
  const bool bBand=InBand(G,Pos.X,Pos.Z,.3);
  bAtDoor|=bBand&&FMath::Abs(Side)<1.2;
  if(Side<-.1){if(P.Kind==EPlace::Saro||(bBand&&Side>-2.5))WasIn[Door]=true;continue;}
  if(Side>.6)WasIn[Door]=false;
  if(Side<.1||!bBand||!WasIn[Door]||!GateOpen(P,Door))continue;
  return Door;
 }
 return -1;
}

void FS47DoorLoop::Step(double Dt)
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F||!F->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 if(!bLinesAdded)AddDevLines();
 StepMotion(Dt);
 if(ThoughtIn>=0&&(ThoughtIn-=Dt)<0){if(M->UI&&!ThoughtText.IsEmpty())M->UI->Thought(ThoughtText,4.f);ThoughtText.Reset();}
 if(RadioOff>=0&&(RadioOff-=Dt)<0&&bRadioOurs){if(M->Audio)M->Audio->AmRadioThree(TOptional<FVector>());bRadioOurs=false;}
 const double Clock=F->Clock();
 const int32 Night=FS47FreeformSession::NightOf(Clock);
 if(!Run.Live())
 {
  // After the big night has passed over: the next quiet minute and a half inside.
  if(F->Happenings&&BigNight!=Night&&F->Happenings->BigLatest()==Night&&!F->Happenings->BigLive())
  {
   if(!bBigSeen){bBigSeen=true;BigAfter=0;}
   if((BigAfter+=Dt)>=BigQuiet&&Arm(TEXT("the big night"),false))BigNight=Night;
  }
  else bBigSeen=false;
  if(Run.Live())return;
  // N1110-7: something after the player: the building they are in is watched, and walking out of it through an open door may start
  // the chase as E on the door would. The building is kept a moment after they are out of it, so the step out is seen.
  if(DevChaseLeft>0)DevChaseLeft=FMath::Max(0.,DevChaseLeft-Dt);
  ES47Taker Who{};FString Title;
  if(!M->Player||!M->Web||!Chased(Who,Title)){Watch=FPlace();return;}
  FPlace At;
  if(PlaceAt(At)&&!At.Same(Watch)){Watch=At;WasIn.Init(false,Watch.Gates.Num());}
  if(Watch.Kind==EPlace::None||M->Web->Area()!=Watch.Area)return;
  bool bNear=false;
  const int32 Door=Crossing(Watch,bNear);
  if(Door<0||!StartChase(Watch))return;
  Start();From=Door;
  const EStep S=Run.Press();
  GateShut(Where,Door);
  Cut(S,Door);
  return;
 }
 // Anything bigger, or a way out it does not hold: over, and the station as it was. The chase is in the dark often enough (the
 // attention visit's power going), so the sky cut is no end to it.
 if(!ThereStill()){Finish(TEXT("left the building's area"));return;}
 if(F->Dead()||(F->Taken&&F->Taken->Busy())||(!bChase&&F->SkyCutLive())){Finish(TEXT("something bigger"));return;}
 if(Run.Stage==EStage::Armed)
 {
  const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
  if(Clock>ArmedUntil||(Source!=TEXT("developer")&&Hour>=LastHour&&Hour<12)||!InStation()){Finish(TEXT("it did not come to a door"));return;}
 }
 if(bChase&&!StepChase(Dt))return;
 if(Pending>=0)
 {
  if((Pending-=Dt)<0){Pending=-1;const int32 Door=PendingFrom;PendingFrom=-1;Cut(PendingStep,Door);}
  return;
 }
 if(!bChase&&Run.Stage==EStage::Running&&!bFalling&&RiseT<0&&(Idle+=Dt)>IdleFor){Finish(TEXT("no door for four minutes"));return;}
 bool bAtDoor=false;
 const int32 Door=Crossing(Where,bAtDoor);
 if(Door>=0)
 {
  if(Run.Stage==EStage::Armed)Start();
  if(Run.Uses==0)From=Door;
  const EStep S=Run.Press();
  if(S==EStep::Out){Break(Door);return;}
  GateShut(Where,Door);
  Cut(S,Door);
  return;
 }
 // Out some other way (the east wing's doors): it lets go. In the chase that is getting away: the thing is gone.
 if(Run.Stage==EStage::Running&&!bAtDoor&&!InPlace()){if(bChase)Break(-1);else Finish(TEXT("out another way"));}
}

// ---------- the chase (N1110-7) ----------

bool FS47DoorLoop::Calm() const{return M&&M->Flow&&M->Flow->bCalmFlash;}

bool FS47DoorLoop::Chased(ES47Taker& Who,FString& Title) const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F)return false;
 // The light looking for them (and the attention visit at SARO, which comes as that light).
 if(F->Taken&&(F->Taken->Stage()==FS47Taken::EStage::Dread||F->Taken->Stage()==FS47Taken::EStage::Flood)){Who=F->Taken->Who();Title=F->Taken->PlaceNow();return true;}
 // A site or field visit's body hunting them, or walking at them near.
 if(F->FarmSite&&(F->FarmSite->VisitStage()==3||F->FarmSite->VisitBodyComing()))
 {
  const int32 K=F->FarmSite->VisitKind();
  Who=K==1?ES47Taker::Crab:K==2?ES47Taker::Tall:ES47Taker::Roswell;Title=F->FarmSite->PlaceTitle();
  return true;
 }
 if(DevChaseLeft>0){Who=ES47Taker::Roswell;Title.Reset();return true;}
 return false;
}

int32 FS47DoorLoop::LastChaseDay() const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 int32 Last=ChaseDay;
 if(F)for(int32 D=F->Day();D>FMath::Max(0,F->Day()-ChaseEvery);--D)if(F->HasFinding(S47DoorLoopLocal::ChaseKey(D))){Last=FMath::Max(Last,D);break;}
 return Last;
}

bool FS47DoorLoop::ChasedThisWeek() const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F)return false;
 if(ChaseDay>0&&F->Day()-ChaseDay<WeekDays)return true;
 for(int32 D=F->Day();D>FMath::Max(0,F->Day()-WeekDays);--D)if(F->HasFinding(S47DoorLoopLocal::ChaseKey(D)))return true;
 return false;
}

bool FS47DoorLoop::ChaseGates(FString& Why,bool bDev) const
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F||!F->Active()||F->Dead()){Why=TEXT("Freeform only");return false;}
 if(Run.Live()){Why=TEXT("the loop is already running");return false;}
 if(!M->Player||!M->World||M->World->bDriving){Why=TEXT("on foot");return false;}
 if(F->Taken&&(F->Taken->Stage()==FS47Taken::EStage::Lift||F->Taken->Stage()==FS47Taken::EStage::Away)){Why=TEXT("being taken");return false;}
 ES47Taker Who{};FString Title;
 if(!Chased(Who,Title)){Why=TEXT("nothing is after the player");return false;}
 if(bDev)return true;
 if(!F->RulesLive()){Why=TEXT("not a live night");return false;}
 if(F->InTutorial()){Why=TEXT("the first shift");return false;}
 const double Clock=F->Clock(),Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 const int32 Night=FS47FreeformSession::NightOf(Clock);
 if(!S47NightTiers::Allows(ES47Allowed::Full,Hour,Night)){Why=TEXT("the full tier only");return false;}
 if(ChaseNight==Night){Why=TEXT("once a night");return false;}
 const int32 LoopToday=LastDay==F->Day()||F->HasFinding(S47DoorLoopLocal::Key(F->Day()))?F->Day():0;
 if(!MayChase(F->Day(),LastChaseDay(),LoopToday)){Why=TEXT("every third night at most, not before the fourth, never after a door loop");return false;}
 if(FS47NightRhythm::Roll(F->Seed(),Night,S47DoorLoopLocal::Salt+2)>=OddsFor(ChasedThisWeek())){Why=TEXT("not tonight (the dice)");return false;}
 return true;
}

bool FS47DoorLoop::StartChase(const FPlace& At)
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 FString Why;
 const bool bDev=DevChaseLeft>0;
 if(!F||At.Kind==EPlace::None||At.Gates.IsEmpty()||!ChaseGates(Why,bDev))return false;
 ES47Taker Who{};FString Title;
 Chased(Who,Title);
 if(!Run.Arm())return false;
 Where=At;bChase=true;bWarned=false;ChaseWho=Who;ChaseTitle=Title.IsEmpty()?At.Title:Title;
 Source=bDev?TEXT("developer chase"):TEXT("chase");DevChaseLeft=0;Idle=0;
 ChaseNight=FS47FreeformSession::NightOf(F->Clock());
 WasIn.Init(false,Where.Gates.Num());
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop: the chase, at %s (%d doors), %s"),*Where.Title,Where.Gates.Num(),*ChaseTitle);
 return true;
}

void FS47DoorLoop::YieldHunt()
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F)return;
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())F->FarmSite->YieldToDoors();
 if(F->Taken&&(F->Taken->Stage()==FS47Taken::EStage::Dread||F->Taken->Stage()==FS47Taken::EStage::Flood))F->Taken->GiveUp();
}

void FS47DoorLoop::Caught()
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 const ES47Taker Who=ChaseWho;
 const FString Title=ChaseTitle;
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop: the chase, it is in (%d uses)"),Run.Uses);
 if(F)ChaseDay=F->Day();
 Restore();
 Run.Reset();Pending=-1;PendingFrom=-1;ThoughtIn=-1;
 // The ordinary taking from here: past its warning, held where they stand.
 if(!F||!F->Taken||!F->Taken->TakeNow(Who,Title))UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop: nothing took over; an ordinary building"));
}

int32 FS47DoorLoop::NearestShut() const
{
 if(!M||!M->Player)return -1;
 int32 Best=-1;double Near=1e9;
 for(int32 I=0;I<Where.Gates.Num();++I)
 {
  if(GateOpen(Where,I))continue;
  const double D=FVector2D::Distance(FVector2D(Where.Gates[I].Mid.X,Where.Gates[I].Mid.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
  if(D<Near){Near=D;Best=I;}
 }
 return Best;
}

void FS47DoorLoop::Footsteps(int32 Gate,int32 Count)
{
 if(!Where.Gates.IsValidIndex(Gate)||Count<=0)return;
 // Along the outside of the wall, past the door, at a walk.
 const FGate& G=Where.Gates[Gate];
 const FVector Along(-G.Out.Z,0,G.Out.X),Out=G.Mid+G.Out*2.2;
 StepsA=Out+Along*3.5;StepsB=Out-Along*3.5;StepsLeft=StepsTotal=Count;StepsIn=.5;
}

void FS47DoorLoop::Knob(int32 Gate,double Seconds,double Wait)
{
 if(!Where.Gates.IsValidIndex(Gate))return;
 if(KnobGate>=0&&KnobGate!=Gate)GateKnob(Where,KnobGate,-1);
 KnobGate=Gate;KnobT=Seconds;KnobWait=Wait;KnobTick=0;
}

bool FS47DoorLoop::StepChase(double Dt)
{
 StepFlood(Dt);
 const double Floor=M->Player?M->Player->FloorY:0.;
 if(StepsLeft>0&&(StepsIn-=Dt)<=0)
 {
  StepsIn=FMath::FRandRange(.5f,.7f);
  const double K=StepsTotal>1?double(StepsTotal-StepsLeft)/double(StepsTotal-1):.5;
  --StepsLeft;
  const FVector At=FMath::Lerp(StepsA,StepsB,K);
  if(M->Audio)M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.75f,.Rate=FMath::FRandRange(.62f,.8f),.At=FVector(At.X,Floor+.1,At.Z)});
 }
 if(KnobGate>=0)
 {
  if(KnobWait>0)KnobWait-=Dt;
  else
  {
   KnobT-=Dt;KnobTick-=Dt;
   const FGate& G=Where.Gates[KnobGate];
   if(KnobTick<=0)
   {
    KnobTick=.45;
    if(M->Audio)M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound{.Gain=.55f,.Rate=FMath::FRandRange(.8f,1.f),.At=G.Mid+FVector(0,Floor+1,0)});
   }
   // A rattle of the leaf in its frame, no more: it stays shut.
   GateKnob(Where,KnobGate,.012+.012*FMath::Sin(KnobT*31.));
   if(KnobT<0){GateKnob(Where,KnobGate,-1);KnobGate=-1;}
  }
 }
 if(Pending>=0||Run.Stage!=EStage::Running||bFalling||RiseT>=0)return true;
 // Standing still is what it waits for: the handle and the steps first, then it is in.
 Idle+=Dt;
 if(!bWarned&&Idle>=ChaseWarn)
 {
  bWarned=true;
  const int32 Near=NearestShut();
  if(Near>=0){Knob(Near,3.,0.);Footsteps(Near,4);}
  Think(S47DoorLoopLocal::AtTheDoor,.4);
 }
 if(Idle>=ChaseIdle){Caught();return false;}
 return true;
}

void FS47DoorLoop::StepFlood(double Dt)
{
 if(!M||!M->Player||!M->Web)return;
 if(!Flood)Flood=MakeShared<FS47OutsideLight>(M);
 FloodT+=Dt;
 const FVector Chest(M->Player->Pos.X,M->Player->FloorY+1.25,M->Player->Pos.Z);
 // Where it stands out there: a place a straight line reaches them from, through a window or a door. Looked for again after
 // each cut, and while none is found.
 if((FloodFind-=Dt)<=0)
 {
  FloodFind=bFloodWay?1e9:2.;
  FVector Way;
  if(FS47OutsideLight::FindWayIn(M,Chest,Way)){FloodFrom=Way;bFloodWay=true;FloodFind=1e9;}
 }
 if(!bFloodWay){Flood->Hide();return;}
 const bool bCalm=Calm();
 double K=FloodK(FloodT,bCalm);
 if(ShadeT>=0)
 {
  ShadeT+=Dt;
  K*=ShadeK(ShadeT,bCalm);
  if(ShadeT>=ShadeFor(bCalm))ShadeT=-1;
 }
 FS47OutsideLight::FFrame Frame;
 Frame.Aim=Chest;Frame.From=FS47OutsideLight::Swept(FloodFrom,Chest,FloodT);Frame.Color=FloodColor(FloodT,bCalm);Frame.K=K;Frame.Seconds=FloodT;
 Flood->Show(Frame);
 Flood->SetAir(FMath::Min(1.,FloodT/2.)*.85,Chest-FVector(0,.15,0),Frame.Color,true);
}

// ---------- the developer's ----------

FString FS47DoorLoop::DevArm()
{
 FString Why;
 if(Run.Live())return Status();
 if(!Gates(Why,true))return TEXT("not now: ")+Why;
 return Arm(TEXT("developer"),true)?TEXT("armed: use the front door or the service door from inside"):TEXT("not now");
}

FString FS47DoorLoop::DevChase()
{
 const FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F||!F->Active()||F->Dead())return TEXT("Freeform only");
 if(Run.Live())return Status();
 DevChaseLeft=DevChaseFor;
 return TEXT("something outside for five minutes: an outer door from inside SARO, the old house or a room in town starts the chase; stand still 40 s and it comes in");
}

void FS47DoorLoop::DevEnd(){DevChaseLeft=0;Finish(TEXT("developer"));}

FString FS47DoorLoop::Status() const
{
 static const TCHAR* const Stages[]={TEXT("none"),TEXT("armed"),TEXT("running"),TEXT("done")};
 FString Why;const bool bMay=Gates(Why,false);
 FString Chase;const bool bChaseMay=ChaseGates(Chase,DevChaseLeft>0);
 return FString::Printf(TEXT("door loop %s, %d of %d door uses (%s%s%s); last day %d; tonight %s; chase: %s (last day %d%s)"),Stages[int32(Run.Stage)],Run.Uses,Cap,Source.IsEmpty()?TEXT("-"):*Source,
  Run.Live()?TEXT(", "):TEXT(""),Run.Live()?*Where.Title:TEXT(""),LastLoopDay(),bMay?TEXT("it may come"):*Why,bChase?TEXT("running"):bChaseMay?TEXT("it may start at a door"):*Chase,LastChaseDay(),DevChaseLeft>0?TEXT(", developer's"):TEXT(""));
}

void FS47DoorLoop::AddDevLines()
{
 bLinesAdded=true;
 if(!M||!M->DevEvents)return;
 static const TCHAR* const Group=TEXT("Door loop (N1110)");
 US47Main* Main=M;
 const auto Have=[Main](const FString& Label){return Main->DevEvents->Events().ContainsByPredicate([&Label](const FS47DevEvents::FEvent& E){return E.Group==Group&&E.Label==Label;});};
 const auto Line=[&](const TCHAR* Label,const TCHAR* Note,TFunction<FString(FS47DoorLoop&)> Fn)
 {
  if(Have(Label))return;
  Main->DevEvents->AddFreeform(Group,Label,Note,[Main,Fn]
  {
   FS47DoorLoop* L=Main->Freeform&&Main->Freeform->DoorLoop?Main->Freeform->DoorLoop.Get():nullptr;
   const FString Said=L?Fn(*L):FString(TEXT("Freeform only"));
   if(Main->UI)Main->UI->Toast(TEXT("Developer event: ")+Said,5);
  });
 };
 Line(TEXT("Door loop (N1110)"),TEXT("past the hour, the week and the dice: the next outer door from inside SARO starts it"),[](FS47DoorLoop& L){return L.DevArm();});
 Line(TEXT("Door loop: where it stands"),TEXT("its stage, the door uses and whether it may come tonight"),[](FS47DoorLoop& L){return L.Status();});
 Line(TEXT("Door loop: chase (N1110)"),TEXT("something outside for five minutes, past the hour, the night and the dice: an outer door from inside SARO, the old house or a room in town starts the chase"),[](FS47DoorLoop& L){return L.DevChase();});
 Line(TEXT("Door loop: end it now"),TEXT("the lights, the eye and the doors as they were, nothing recorded"),[](FS47DoorLoop& L){L.DevEnd();return L.Status();});
}

// ---------- the self-test ----------

bool FS47DoorLoop::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s door loop: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 {
  FMachine Mc;
  Check(Mc.Press()==EStep::Normal&&Mc.Uses==0&&!Mc.Live(),TEXT("not armed, a door is a door"));
  Check(Mc.Arm()&&!Mc.Arm()&&Mc.Stage==EStage::Armed&&Mc.Live(),TEXT("armed once, not twice"));
  TArray<EStep> Got;for(int32 K=0;K<Cap;++K)Got.Add(Mc.Press());
  const TArray<EStep> Want={EStep::Side,EStep::Side,EStep::Side,EStep::Ceiling,EStep::Floor,EStep::Out};
  Check(Got==Want,TEXT("three side doors, the ceiling, the floor, then out"));
  Check(Mc.Stage==EStage::Done&&Mc.Uses==Cap&&!Mc.Live()&&Mc.Press()==EStep::Normal&&Mc.Uses==Cap,TEXT("after the way out, doors are doors again"));
  Mc.Reset();
  Check(Mc.Stage==EStage::None&&Mc.Uses==0&&Mc.Arm(),TEXT("reset is a clean start"));
  Mc.Press();Mc.Press();Mc.Reset();
  Check(Mc.Stage==EStage::None&&Mc.Uses==0&&Mc.Press()==EStep::Normal,TEXT("reset mid-loop leaves nothing running"));
 }
 {
  FMachine Mc;Mc.Arm();int32 Outs=0,Presses=0;
  while(Mc.Live()&&Presses<100){++Presses;Outs+=Mc.Press()==EStep::Out?1:0;}
  Check(Presses==Cap&&Outs==1&&Cap==6,TEXT("hard cap: six door uses at most, and one way out"));
  Check(StepFor(0)==EStep::Normal&&StepFor(Cap)==EStep::Out&&StepFor(Cap+5)==EStep::Out,TEXT("past the cap is always out"));
 }
 {
  bool bDown=true;for(int32 U=1;U<Cap-1;++U)bDown&=LightAt(U+1)<LightAt(U);
  Check(bDown&&LightAt(0)==1.&&LightAt(Cap)==1.&&LightAt(Cap-1)>=.3,TEXT("the lights a step lower each time, never black, and back after"));
  bool bRange=true;
  for(int32 Seed=1;Seed<400;Seed+=7)for(int32 Night=2;Night<30;Night+=3)
  {
   int32 Sum=FinalMinutes(Seed,Night);for(int32 U=1;U<Cap;++U)Sum+=JumpMinutes(U);
   const int32 Miss=MissingMinutes(Seed,Night);
   bRange&=Miss>=10&&Miss<=25&&Sum==Miss&&FinalMinutes(Seed,Night)>=1;
  }
  Check(bRange,TEXT("missing time 10 to 25 minutes, the clock's jumps and the way out add up to it"));
  Check(MissingMinutes(5,3)==MissingMinutes(5,3),TEXT("the same night, the same missing time"));
 }
 Check(!MayLoop(1,0)&&!MayLoop(2,0)&&!MayLoop(3,0)&&MayLoop(4,0)&&!MayLoop(5,3)&&!MayLoop(9,3)&&MayLoop(10,3),TEXT("N1110: not before the fourth night, at most once a week"));
 Check(S47DoorLoopLocal::BigThingKey(TEXT("taken-5-70200"),5,nullptr)&&!S47DoorLoopLocal::BigThingKey(TEXT("taken-5-70200"),6,nullptr)&&!S47DoorLoopLocal::BigThingKey(TEXT("taken-50-1"),5,nullptr)&&!S47DoorLoopLocal::BigThingKey(TEXT("door-loop-d5"),5,nullptr),TEXT("N1110 (pace): a taking on this day, and only this day, is the night's one big thing"));
 {
  bool bThree=true;
  for(int32 I=0;I<3;++I)bThree&=S47Script::LintText(S47DoorLoopLocal::FrontDoors[I]).IsEmpty()&&!FString(S47DoorLoopLocal::FrontDoors[I]).IsEmpty()&&(I==0||FCString::Strcmp(S47DoorLoopLocal::FrontDoors[I],S47DoorLoopLocal::FrontDoors[I-1])!=0);
  Check(bThree,TEXT("N1110 (pace): three different arm thoughts"));
 }
 // ---- N1110-7: the chase ----
 {
  // SARO through the building's door set: the same planes, bands and facing as the two doors had by themselves.
  const FPlace S=SaroPlace();
  bool bSame=S.Kind==EPlace::Saro&&S.Gates.Num()==2&&S.bUp&&S.Gates[0].Id==TEXT("exitDoor")&&S.Gates[1].Id==TEXT("doorEast");
  for(int32 I=0;I<2&&bSame;++I)
  {
   const auto& D=S47DoorLoopLocal::Doors[I];const FGate& G=S.Gates[I];
   bSame&=FMath::IsNearlyEqual(YawIn(G),D.Out*UE_DOUBLE_HALF_PI,1e-9);
   for(double X=-8.;X<8.;X+=.37)for(double Z=-.5;Z<8.;Z+=.29)
   {
    bSame&=FMath::IsNearlyEqual(SideOf(G,X,Z),(X-D.Plane)*D.Out,1e-9);
    if(FMath::Abs(Z-(D.Z0-.3))>1e-6&&FMath::Abs(Z-(D.Z1+.3))>1e-6)bSame&=InBand(G,X,Z,.3)==(Z>=D.Z0-.3&&Z<=D.Z1+.3);
   }
  }
  Check(bSame,TEXT("chase: SARO's two doors read the same through the building's door set"));
  FS47DoorLoop Two(nullptr);Two.Where=S;
  FS47DoorLoop One(nullptr);One.Where.Gates.SetNum(1);
  Check(Two.OtherOf(0)==1&&Two.OtherOf(1)==0&&One.OtherOf(0)==0,TEXT("chase: the other door of two, the same door of a room with one"));
 }
 {
  // The light: calm flashes never flash; otherwise bursts of strobe no quicker than seven a second, and all four colours come.
  constexpr double Dt=1./240.;
  double CalmStep=0,CalmTop=0,CalmLow=9;int32 MostInSecond=0,Bursts=0;
  TArray<double> Ons;double Was=FloodK(0,false);
  for(double T=Dt;T<40.;T+=Dt)
  {
   const double C0=FloodK(T-Dt,true),C1=FloodK(T,true);
   CalmStep=FMath::Max(CalmStep,FMath::Abs(C1-C0));CalmTop=FMath::Max(CalmTop,C1);if(T>2)CalmLow=FMath::Min(CalmLow,C1);
   const double K=FloodK(T,false);
   if(Was<.5&&K>1.5)Ons.Add(T);
   Was=K;
  }
  for(int32 I=0;I<Ons.Num();++I){int32 N=0;for(const double O:Ons)N+=O>=Ons[I]&&O<Ons[I]+1.?1:0;MostInSecond=FMath::Max(MostInSecond,N);if(I==0||Ons[I]-Ons[I-1]>2.)++Bursts;}
  Check(CalmStep<.01&&CalmTop<1.35&&CalmLow>1.&&ShadeK(0,true)==1.&&ShadeK(ShadeFor(true)*.5,true)>.4,TEXT("chase: with calm flashes the light only breathes, and the shape crossing it is a slow dimming"));
  Check(Bursts>=6&&MostInSecond>=1&&MostInSecond<=3&&FloodK(1.,false)>1.1&&ShadeK(ShadeFor(false)*.5,false)<.1,TEXT("chase: otherwise strong, with bursts of strobe at most three a second, and the shape all but blacks it"));
  bool bColours=true;
  for(const bool bCalm:{false,true})for(const FLinearColor& Want:S47DoorLoopLocal::FloodColors)
  {
   double Near=9;
   for(double T=0;T<40.;T+=Dt)
   {
    const FLinearColor C=FloodColor(T,bCalm);
    Near=FMath::Min(Near,double(FMath::Abs(C.R-Want.R)+FMath::Abs(C.G-Want.G)+FMath::Abs(C.B-Want.B)));
   }
   bColours&=Near<.03;
  }
  Check(bColours,TEXT("chase: white, amber, blue and green all come, calm or not"));
 }
 Check(!MayChase(1,0,0)&&!MayChase(2,0,0)&&!MayChase(3,0,0)&&MayChase(4,0,0)&&!MayChase(4,3,0)&&!MayChase(5,3,0)&&MayChase(6,3,0)&&!MayChase(6,3,6)&&MayChase(6,3,5),TEXT("chase: none before the fourth night (N1110 pace), every third at most, never the night of a door loop"));
 Check(OddsFor(false)==1.&&OddsFor(true)==ChaseOdds&&ChaseOdds>0&&ChaseOdds<1,TEXT("chase: the first eligible hunt of a week chases for sure (odds 1), then the draw (.65)"));
 Check(ChaseWarn<ChaseIdle&&ChaseIdle<IdleFor&&ChaseOdds>0&&ChaseOdds<1&&DevChaseFor>0,TEXT("chase: the handle before it is in, both well before the loop's own idle end"));
 {
  TArray<FString> Words={S47DoorLoopLocal::FrontDoors[0],S47DoorLoopLocal::FrontDoors[1],S47DoorLoopLocal::FrontDoors[2],ChaseThought(1),ChaseThought(2),ChaseThought(3),S47DoorLoopLocal::AtTheDoor,TEXT("It's gone. How long was I in there?"),ChaseRecord(TEXT("the old house"),17),ChaseRecord(TEXT("SARO"),12),ChaseRecord(FString(),20)};
  bool bClean=true;
  for(const FString& W:Words)bClean&=!W.IsEmpty()&&S47Script::LintText(W).IsEmpty()&&!W.Contains(TEXT("taken"));
  Check(bClean&&FString(ChaseThought(4)).IsEmpty()&&ChaseRecord(TEXT("the old house"),17).StartsWith(TEXT("The old house,")),TEXT("chase: its words keep to the spoiler limits"));
 }
 {
  TArray<FString> Words={ThoughtFor(1,true),ThoughtFor(1,false),ThoughtFor(2,true),ThoughtFor(3,true),RecordText(17,true),RecordText(12,false),TEXT("How long was I in there?")};
  bool bClean=true;
  for(const FString& W:Words)bClean&=S47Script::LintText(W).IsEmpty()&&!W.Contains(TEXT("taken"));
  Check(bClean&&FString(ThoughtFor(4,true)).IsEmpty(),TEXT("its words keep to the spoiler limits"));
 }
 {
  FS47DoorLoop Old(nullptr);Old.Begin();
  Check(!Old.Active()&&Old.UsesNow()==0&&Old.StageNow()==EStage::None&&!Old.Use(TEXT("exitDoor"))&&!Old.TakeVisit(),TEXT("without a world nothing starts, and a door is a door"));
  Old.Run.Arm();Old.Run.Press();Old.Stop();
  Check(!Old.Active()&&Old.UsesNow()==0,TEXT("Stop (a load, Continue) mid-loop: nothing left running"));
  Check(!Old.Use(TEXT("ff:olddoor:north0"))&&!Old.Use(TEXT("access:H02"))&&!Old.Chasing()&&Old.DevChase()==TEXT("Freeform only"),TEXT("chase: without a world no chase starts, at the old house or in town"));
  Old.Run.Arm();Old.bChase=true;Old.Where=SaroPlace();
  Check(Old.Chasing(),TEXT("chase: a running chase is seen as one"));
  Old.Stop();
  Check(!Old.Chasing()&&!Old.Active(),TEXT("chase: Stop mid-chase leaves nothing running"));
 }
 return OK;
}
