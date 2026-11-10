#include "S47DoorLoop.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47DevEvents.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47NightTiers.h"
#include "S47SaroSave.h"
#include "S47Script.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DoorLoop,Log,All);

// N1110 (N1110-6): see S47DoorLoop.h.

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
 case 3:return TEXT("I went out. I went OUT.");
 default:return TEXT("");
 }
}

FString FS47DoorLoop::RecordText(int32 Minutes,bool bFromFront)
{
 return FString::Printf(TEXT("SARO, in the small hours. I went out the %s and came in at the %s. Then again, the other way, and again. The lights were lower each time. Once I was up by the ceiling of the corridor and fell, and once I came up through the floor. When a door finally let me out, the clock was about %d minutes ahead of where it should have been."),
  bFromFront?TEXT("front door"):TEXT("service door"),bFromFront?TEXT("service door"):TEXT("front door"),Minutes);
}

// ---------- the session's ----------

void FS47DoorLoop::Begin()
{
 // Nothing of a loop is kept: a Continue or a load mid-loop is an ordinary station (Stop has put the lights and the eye back).
 Restore();
 Run.Reset();Source.Reset();LastDay=0;BigNight=0;PendingFrom=-1;PendingStep=EStep::Normal;From=0;
 ArmedUntil=Idle=BigAfter=0;Pending=ThoughtIn=RadioOff=-1;ThoughtText.Reset();WasIn[0]=WasIn[1]=false;bBigSeen=false;
}

void FS47DoorLoop::Stop()
{
 Restore();
 Run.Reset();Pending=-1;PendingFrom=-1;ThoughtIn=-1;
}

void FS47DoorLoop::Restore()
{
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
 if(!MayLoop(F->Day(),LastLoopDay())){Why=TEXT("once a week, never the first night");return false;}
 const double Clock=F->Clock(),Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 if(!S47NightTiers::Allows(ES47Allowed::Full,Hour,FS47FreeformSession::NightOf(Clock))||Hour>=LastHour){Why=TEXT("the full tier only, to half past five");return false;}
 if(F->Director&&(F->Director->DangerTonight()>0||F->Director->Busy())){Why=TEXT("one danger a night");return false;}
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
 WasIn[0]=InsideOf(0);WasIn[1]=InsideOf(1);
 // Quiet first: the night outside stops all at once, then three soft knocks at the front door.
 if(M->Audio&&M->Audio->Night.IsValid())M->Audio->Night->Hush(12,.04,.12,3.5);
 if(M->Audio)
 {
  const FS47WebItem* It=M->Web->Item(S47DoorLoopLocal::Doors[0].Id);
  const FVector At=It?M->Web->ObjAt(It->Node).WorldPosition():FVector(S47DoorLoopLocal::Doors[0].Plane,1.2,5.7);
  for(int32 K=0;K<3;++K)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.55f,.Rate=.9f,.At=At,.When=4.f+.5f*K});
 }
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
 for(int32 I=0;I<2;++I)if(Id==S47DoorLoopLocal::Doors[I].Id)return I;
 return -1;
}

bool FS47DoorLoop::InsideOf(int32 Door) const
{
 if(!M||!M->Player||Door<0||Door>1)return false;
 const auto& D=S47DoorLoopLocal::Doors[Door];
 return (M->Player->Pos.X-D.Plane)*D.Out<0;
}

bool FS47DoorLoop::Arrival(int32 Door,double& X,double& Z,double& Yaw) const
{
 if(!M||!M->Player||Door<0||Door>1)return false;
 const auto& D=S47DoorLoopLocal::Doors[Door];
 Z=.5*(D.Z0+D.Z1);Yaw=D.Out*UE_DOUBLE_HALF_PI; // facing in, away from the door, as if just come in
 for(const double In:{1.6,1.3,1.,.75})
 {
  X=D.Plane-D.Out*In;
  if(M->Player->Walkable(X,Z))return true;
 }
 return false;
}

bool FS47DoorLoop::Use(const FString& Id)
{
 if(!Run.Live())return false;
 const int32 Door=DoorOf(Id);
 if(Door<0||!M||!M->Web)return false;
 if(Pending>=0)return true; // the cut is coming: nothing more of this door
 if(!InsideOf(Door)||M->Web->IsDoorOpen(Id))return false; // from outside, or shutting it: an ordinary door
 if(Run.Stage==EStage::Armed)Start();
 if(Run.Uses==0)From=Door;
 const EStep S=Run.Press();
 if(S==EStep::Out){Break(Door);return false;} // it opens, as a door does
 // The latch goes; the door does not move. A moment later the player is somewhere else.
 const FS47WebItem* It=M->Web->Item(Id);
 if(M->Audio)M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound{.Gain=.5f,.At=It?TOptional<FVector>(M->Web->ObjAt(It->Node).WorldPosition()):TOptional<FVector>()});
 Pending=CutAfter;PendingStep=S;PendingFrom=Door;
 return true;
}

void FS47DoorLoop::Start()
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 if(!F)return;
 LastDay=F->Day();Idle=0;
 if(F->Director&&Source!=TEXT("developer"))F->Director->NoteDanger(); // the night's one danger
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop begins (%s) at %.2f %.2f"),*Source,M->Player?M->Player->Pos.X:0.,M->Player?M->Player->Pos.Z:0.);
}

void FS47DoorLoop::Cut(EStep S,int32 FromDoor)
{
 using namespace S47DoorLoopLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||!M->Player||!M->Web){Finish(TEXT("no world"));return;}
 const int32 Other=1-FromDoor,N=Run.Uses;
 double X=0,Z=0,Yaw=0;
 bool bSpot=Arrival(Other,X,Z,Yaw);
 if(S==EStep::Ceiling&&M->Player->Walkable(CeilX,CeilZ)){X=CeilX;Z=CeilZ;Yaw=-UE_DOUBLE_HALF_PI;bSpot=true;}
 if(!bSpot){Finish(TEXT("nowhere to stand"));return;} // never stuck: an ordinary station at once
 M->Player->Place(X,Z,Yaw);
 if(S==EStep::Side)
 {
  // The other door has just let the player in: it swings shut behind them, with its own sound.
  M->Web->SetDoorOpen(Doors[Other].Id,true,true);
  M->Web->SetDoorOpen(Doors[Other].Id,false,false);
 }
 else if(S==EStep::Ceiling)
 {
  bFalling=true;FallHang=Hang;FallV=0;M->Player->Lift=M->Player->bHeadMotion?FallFrom:FallFromCalm;
 }
 else if(S==EStep::Floor)
 {
  RiseSecs=M->Player->bHeadMotion?1.2:1.8;RiseDepth=1.75;RiseT=0;M->Player->Lift=-RiseDepth;
  if(M->Audio)M->Audio->Beep(46.f,float(RiseSecs)+.6f,.06f,0.f,TEXT("sine"));
 }
 // A step dimmer, the clock on, the night still holding its breath.
 M->Web->SetLightsScale(float(LightAt(N)),float(FMath::Max(.5,LightAt(N))));bDimmed=true;
 if(JumpMinutes(N)>0)F->SpendTime(60.*JumpMinutes(N));
 if(M->Audio&&M->Audio->Night.IsValid())M->Audio->Night->Hush(8,.03,.12,3.5);
 if(N==2&&M->Audio&&!M->Audio->RadioOn())
 {
  const FS47Obj Set=M->Web->Obj(TEXT("room.objs.radio"));
  if(Set){M->Audio->AmRadioThree(Set.WorldPosition());bRadioOurs=true;RadioOff=4.5;}
 }
 if(N==3&&M->Audio)
 {
  const FS47WebItem* Phone=M->Web->Item(TEXT("phone"));
  FS47Sound Ring;Ring.Gain=.45f;Ring.When=1.2f;Ring.Dur=1.1f;
  if(Phone)Ring.At=M->Web->ObjAt(Phone->Node).WorldPosition();
  M->Audio->Sfx(TEXT("phoneRing"),Ring);
 }
 const FString Said=ThoughtFor(N,From==0);
 if(!Said.IsEmpty())Think(Said,.9);
 Idle=0;WasIn[0]=WasIn[1]=false; // a cut is never a walk out
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop %d/%d: step %d, from %s, now at %.2f %.2f"),N,Cap,int32(S),Doors[FromDoor].Id,X,Z);
}

void FS47DoorLoop::Break(int32 FromDoor)
{
 FS47FreeformSession* F=S47DoorLoopLocal::Session(M);
 Restore();
 if(F)
 {
  const int32 Night=FS47FreeformSession::NightOf(F->Clock());
  F->SpendTime(60.*FinalMinutes(F->Seed(),Night));
  const FString Rec=S47DoorLoopLocal::Key(F->Day());
  if(!F->HasFinding(Rec))F->RecordEvidence(Rec,RecordText(MissingMinutes(F->Seed(),Night),From==0));
  if(F->Director)F->Director->RestFor(RestAfter); // relief: quiet minutes after it
  LastDay=F->Day();
 }
 Think(TEXT("How long was I in there?"),2.2);
 UE_LOG(LogS47DoorLoop,Display,TEXT("S47: door loop broken at the %s after %d uses"),S47DoorLoopLocal::Doors[FMath::Clamp(FromDoor,0,1)].Id,Run.Uses);
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
  return;
 }
 // Anything bigger, or a way out it does not hold: over, and the station as it was.
 const bool bSaro=M->Web&&M->World&&M->Player&&M->Web->Area()==TEXT("saro")&&M->World->Area==TEXT("saro")&&!M->World->bDriving;
 if(!bSaro){Finish(TEXT("left SARO"));return;}
 if(F->Dead()||(F->Taken&&F->Taken->Busy())||F->SkyCutLive()){Finish(TEXT("something bigger"));return;}
 if(Run.Stage==EStage::Armed)
 {
  const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
  if(Clock>ArmedUntil||(Source!=TEXT("developer")&&Hour>=LastHour&&Hour<12)||!InStation()){Finish(TEXT("it did not come to a door"));return;}
 }
 if(Pending>=0)
 {
  if((Pending-=Dt)<0){Pending=-1;const int32 Door=PendingFrom;PendingFrom=-1;Cut(PendingStep,Door);}
  return;
 }
 if(Run.Stage==EStage::Running&&!bFalling&&RiseT<0&&(Idle+=Dt)>IdleFor){Finish(TEXT("no door for four minutes"));return;}
 // Walked out through a door standing open: the same as E on it. WasIn: last seen clearly on the inside of that door's plane.
 const FVector& P=M->Player->Pos;
 bool bAtDoor=false;
 for(int32 Door=0;Door<2;++Door)
 {
  const auto& D=S47DoorLoopLocal::Doors[Door];
  const double Side=(P.X-D.Plane)*D.Out;
  const bool bBand=P.Z>=D.Z0-.3&&P.Z<=D.Z1+.3;
  bAtDoor|=bBand&&FMath::Abs(Side)<1.2;
  if(Side<-.1){WasIn[Door]=true;continue;}
  if(Side>.6)WasIn[Door]=false;
  if(Side<.1||!bBand||!WasIn[Door]||!M->Web->IsDoorOpen(D.Id))continue;
  if(Run.Stage==EStage::Armed)Start();
  if(Run.Uses==0)From=Door;
  const EStep S=Run.Press();
  if(S==EStep::Out){Break(Door);return;}
  M->Web->SetDoorOpen(D.Id,false,true);
  Cut(S,Door);
  return;
 }
 // Out some other way (the east wing's doors): it lets go.
 if(Run.Stage==EStage::Running&&!bAtDoor&&!InStation())Finish(TEXT("out another way"));
}

// ---------- the developer's ----------

FString FS47DoorLoop::DevArm()
{
 FString Why;
 if(Run.Live())return Status();
 if(!Gates(Why,true))return TEXT("not now: ")+Why;
 return Arm(TEXT("developer"),true)?TEXT("armed: use the front door or the service door from inside"):TEXT("not now");
}

void FS47DoorLoop::DevEnd(){Finish(TEXT("developer"));}

FString FS47DoorLoop::Status() const
{
 static const TCHAR* const Stages[]={TEXT("none"),TEXT("armed"),TEXT("running"),TEXT("done")};
 FString Why;const bool bMay=Gates(Why,false);
 return FString::Printf(TEXT("door loop %s, %d of %d door uses (%s); last day %d; tonight %s"),Stages[int32(Run.Stage)],Run.Uses,Cap,Source.IsEmpty()?TEXT("-"):*Source,LastLoopDay(),bMay?TEXT("it may come"):*Why);
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
 Check(!MayLoop(1,0)&&MayLoop(2,0)&&!MayLoop(5,3)&&!MayLoop(9,3)&&MayLoop(10,3),TEXT("never the first night, at most once a week"));
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
 }
 return OK;
}
