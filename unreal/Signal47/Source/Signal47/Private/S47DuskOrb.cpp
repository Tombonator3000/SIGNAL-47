#include "S47DuskOrb.h"

namespace
{
 constexpr double Day=86400.,Dusk=18.5*3600.,DuskEnd=19.25*3600.,Grace=120.,NightWindow=3600.;
 bool ValidClock(double Clock){return FMath::IsFinite(Clock)&&Clock>=0&&Clock<=86400000000.;}
 double DayStart(double Clock){return FMath::FloorToDouble(Clock/Day)*Day;}
 // Evenings to the next visit once it has been photographed. Written out, not drawn: it has to survive Continue.
 int32 PhotographedGap(int32 Attempts)
 {
  static const int32 Gaps[6]={3,2,4,3,4,2};
  return Gaps[FMath::Abs(Attempts)%6];
 }
 double NextOpportunity(double Clock)
 {
  const double Start=DayStart(Clock);
  const double Candidate=FMath::Max(Start+Dusk,Clock+Grace);
  return Candidate<=Start+DuskEnd?Candidate:Start+Day+Dusk;
 }
 // The hour of a night's visit and the nights to the next, written out like the evenings above: they have to survive Continue.
 // A visit lasts five real minutes, about an hour and a half of the clock: the latest is over before two, when their hours begin.
 double NightSlot(int32 Visits)
 {
  static const double Hours[6]={21.25,22.5,20.25,23.25,21.75,20.75};
  return Hours[FMath::Abs(Visits)%6]*3600.;
 }
 int32 NightGap(int32 Visits)
 {
  static const int32 Gaps[6]={2,1,3,2,1,3};
  return Gaps[FMath::Abs(Visits)%6];
 }
 double NextNight(double Clock,int32 Visits)
 {
  const double Start=DayStart(Clock);const double Slot=Start+NightSlot(Visits);
  const double Candidate=FMath::Max(Slot,Clock+Grace);
  return Candidate<=Slot+NightWindow?Candidate:Slot+Day;
 }
}
void FS47DuskOrb::Begin(const S47J::FObj& Saved)
{
 DueClock=-1;Attempts=0;bPhotographed=false;NightClock=-1;NightVisits=0;
 if(!Saved)return;
 // Older saves have neither: the first Update schedules the coming night.
 const double Night=S47J::Num(Saved,TEXT("nightDue"),-1);
 if(ValidClock(Night))NightClock=Night;
 NightVisits=FMath::Clamp(S47J::Int(Saved,TEXT("nightVisits")),0,1000000);
 const double Due=S47J::Num(Saved,TEXT("dueClock"),-1);
 if(ValidClock(Due))DueClock=Due;
 Attempts=FMath::Clamp(S47J::Int(Saved,TEXT("attempts")),0,1000000);
 bPhotographed=S47J::Bool(Saved,TEXT("photographed"));
}
S47J::FObj FS47DuskOrb::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("dueClock"),DueClock);S47J::Set(O,TEXT("attempts"),Attempts);S47J::Set(O,TEXT("photographed"),bPhotographed);S47J::Set(O,TEXT("nightDue"),NightClock);S47J::Set(O,TEXT("nightVisits"),NightVisits);return O;
}
bool FS47DuskOrb::Update(double Clock,bool bTutorial)
{
 if(bTutorial||!ValidClock(Clock))return false;
 // A night slept through or missed is no more a witnessed event than a dusk is.
 const bool bNightMoved=NightClock<0||Clock>NightClock+NightWindow;
 if(bNightMoved)NightClock=NextNight(Clock,NightVisits);
 if(DueClock<0){DueClock=NextOpportunity(Clock);return true;}
 // A skipped dusk is not a witnessed event. Reschedule without inventing an
 // encounter, evidence, warning or payment, including after a sleep/Continue.
 if(Clock>DayStart(DueClock)+DuskEnd){DueClock=NextOpportunity(Clock)+(bPhotographed?(PhotographedGap(Attempts)-1)*Day:0.);return true;}
 return bNightMoved;
}
bool FS47DuskOrb::Ready(double Clock) const
{
 return DueClock>=0&&ValidClock(Clock)&&Clock>=DueClock&&Clock<=DayStart(DueClock)+DuskEnd;
}
bool FS47DuskOrb::ReadyNight(double Clock) const
{
 return NightClock>=0&&ValidClock(Clock)&&Clock>=NightClock&&Clock<=NightClock+NightWindow;
}
void FS47DuskOrb::Started(double Clock)
{
 if(!ValidClock(Clock))return;
 // A night's visit takes nothing from the dusk's schedule, its attempts or its photographic record.
 if(ReadyNight(Clock))
 {
  NightVisits=FMath::Min(1000000,NightVisits+1);
  NightClock=DayStart(NightClock)+NightGap(NightVisits)*Day+NightSlot(NightVisits);
  return;
 }
 Attempts=FMath::Min(1000000,Attempts+1);
 DueClock=DayStart(Clock)+(bPhotographed?PhotographedGap(Attempts):1)*Day+Dusk;
}
void FS47DuskOrb::Photographed()
{
 if(bPhotographed)return;
 bPhotographed=true;
 // This evening's visit has already set tomorrow's: put that one back too.
 if(DueClock>=0)DueClock+=(PhotographedGap(Attempts)-1)*Day;
}
namespace
{
 constexpr double Leg=11.,Hold=.6; // of each 11 s leg: 60 % sniffing, 40 % travelling
 double RouteTime(double Age,int32 Index){return FMath::Clamp(FMath::IsFinite(Age)?Age:0.,0.,FS47DuskOrb::Duration)+Index*2.3;}
 // Which of the stops an orb goes to on this leg. Orb 0 keeps to S-03, orb 1 to S-02, orb 2 goes between them; no leg is too long
 // for 4.4 s of travel, and a person watching orb 0 from nine metres south at its first stops is outside the flight range of all.
 int32 RouteStop(int32 Stop,int32 Index)
 {
  static const int32 Routes[3][6]={{0,2,1,0,2,1},{5,3,4,5,3,4},{4,5,3,0,2,1}};
  return Routes[FMath::Abs(Index)%3][(FMath::Max(Stop,1)-1)%6];
 }
 // The spot one orb investigates next, from its home: the first is in the open corridor, the rest are the equipment.
 FVector Spot(int32 Stop,int32 Index)
 {
  if(Stop==0)return Index==0?FVector::ZeroVector:FVector(Index==1?-5.:6.,.3*Index,Index==1?3.:-3.);
  const auto& S=FS47DuskOrb::Stops()[RouteStop(Stop,Index)];
  // Two lights at one thing hang side by side, not in one place.
  const FVector Beside=Index==2?FVector(-1.1,.5,.9):FVector::ZeroVector;
  return FVector(S.X-FS47DuskOrb::HomeX,S.Up-FS47DuskOrb::HomeUp,S.Z-FS47DuskOrb::HomeZ)+Beside;
 }
}
TArrayView<const FS47DuskOrb::FStop> FS47DuskOrb::Stops()
{
 // Beside the mast under the dish, at the door of the box, under the low rim of the reflector at its usual az 026, el 32.
 static const FStop Data[6]={
  {TEXT("mast:S-03"),TEXT("S-03 mast"),6.4,-43.6,2.6},
  {TEXT("field:S-03"),TEXT("S-03 junction cabinet"),4.9,-41.,1.4},
  {TEXT("mast:S-03"),TEXT("S-03 reflector rim"),12.4,-47.9,4.},
  {TEXT("mast:S-02"),TEXT("S-02 mast"),-13.,-49.7,2.6},
  {TEXT("field:S-02"),TEXT("S-02 drive box"),-10.1,-54.6,1.5},
  {TEXT("mast:S-02"),TEXT("S-02 reflector rim"),-13.2,-57.7,4.}};
 return MakeArrayView(Data);
}
int32 FS47DuskOrb::StopAt(double Age,int32 Index)
{
 const double T=RouteTime(Age,Index);const int32 Stop=int32(T/Leg);
 return Stop<1||FMath::Fmod(T,Leg)/Leg>=Hold?-1:RouteStop(Stop,Index);
}
FVector FS47DuskOrb::Position(const FVector& Anchor,double Age,int32 Index)
{
 // No camera, player position or random draw enters this function; looking away
 // never repositions it. It holds at a spot, sniffs in small circles, then moves on.
 const double T=RouteTime(Age,Index);
 const int32 Stop=int32(T/Leg);const double U=FMath::Fmod(T,Leg)/Leg;
 const double Move=U<Hold?0.:FMath::SmoothStep(0.,1.,(U-Hold)/(1-Hold));
 const FVector Base=FMath::Lerp(Spot(Stop,Index),Spot(Stop+1,Index),Move);
 // The circle fades to nothing at T=0 so the first orb starts exactly on its anchor.
 const double Sniff=FMath::Min(T/4.,1.)*(1-Move);
 const FVector Near(.45*FMath::Sin(T*1.9+Index),.22*FMath::Sin(T*3.1+Index*2.),.45*FMath::Cos(T*1.6+Index));
 return Anchor+Base+Near*Sniff;
}
double FS47DuskOrb::FleeRise(double FleeAge)
{
 const double T=FMath::Max(0.,FMath::IsFinite(FleeAge)?FleeAge:0.);
 return .5*150.*T*T;
}
double FS47DuskOrb::Visibility(double Age)
{
 if(!FMath::IsFinite(Age))return 0;
 return FMath::Min(FMath::Clamp(Age/3.,0.,1.),FMath::Clamp((Duration-Age)/8.,0.,1.));
}
bool FS47DuskOrb::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Pass,const TCHAR* Text){Lines.Add(FString(Pass?TEXT("PASS "):TEXT("FAIL "))+Text);OK&=Pass;};
 FS47DuskOrb S;S.Begin(nullptr);
 Check(!S.Update(8*3600.,true)&&!S.Ready(Dusk),TEXT("Dusk orb does not schedule during tutorial"));
 Check(S.Update(9*3600.,false)&&!S.Ready(Dusk-1)&&S.Ready(Dusk),TEXT("Early tutorial completion schedules the coming dusk"));
 S.Started(Dusk);FS47DuskOrb R;R.Begin(S47J::Read(S47J::Write(S.Save())));
 Check(!R.Ready(Dusk)&&R.Ready(Day+Dusk),TEXT("Consumed dusk survives Continue without restarting or losing next opportunity"));
 R.Photographed();S.Begin(R.Save());Check(!S.Ready(Day+Dusk)&&(S.Ready(2*Day+Dusk)||S.Ready(3*Day+Dusk)||S.Ready(4*Day+Dusk))&&S.WasPhotographed(),TEXT("After a photograph the lights come back every few evenings; the record of it survives Continue"));
 S.Begin(nullptr);S.Update(22*3600.,false);Check(!S.Ready(22*3600.)&&S.Ready(Day+Dusk),TEXT("Late tutorial completion waits for the next dusk"));
 S.Begin(nullptr);S.Update(18.75*3600.,false);Check(!S.Ready(18.75*3600.)&&S.Ready(18.75*3600.+Grace),TEXT("Tutorial completed at dusk gets a short saved grace period"));
 S.Begin(nullptr);S.Update(8*3600.,false);Check(S.Update(Day+8*3600.,false)&&!S.Ready(Day+8*3600.)&&S.Ready(Day+Dusk),TEXT("Sleeping past dusk reschedules without manufacturing an encounter"));
 S.Begin(nullptr);S.Update(9*3600.,false);const double FirstNight=S.NextNightClock();const double NightHour=FMath::Fmod(FirstNight,Day)/3600.;
 Check(!S.ReadyNight(Dusk)&&!S.ReadyNight(FirstNight-1)&&S.ReadyNight(FirstNight)&&NightHour>=20&&NightHour<24&&S.Ready(Dusk),TEXT("Lights have their own hour in the dark as well as the dusk"));
 S.Started(FirstNight);FS47DuskOrb N;N.Begin(S47J::Read(S47J::Write(S.Save())));
 Check(!N.ReadyNight(FirstNight)&&!N.ReadyNight(FirstNight+60)&&N.ReadyNight(N.NextNightClock())&&N.NextNightClock()>FirstNight+.5*Day&&N.NightVisitCount()==1&&N.Ready(Dusk),TEXT("A night's visit is consumed, survives Continue and leaves the dusk's own schedule alone"));
 S.Begin(nullptr);S.Update(9*3600.,false);Check(S.Update(Day+8*3600.,false)&&S.NightVisitCount()==0&&!S.ReadyNight(Day+8*3600.)&&S.NextNightClock()>Day+8*3600.&&S.NextNightClock()<2*Day,TEXT("Sleeping through a night's hour reschedules without inventing a visit"));
 auto Old=S47J::New();S47J::Set(Old,TEXT("dueClock"),Dusk);S47J::Set(Old,TEXT("attempts"),1);S47J::Set(Old,TEXT("photographed"),true);
 S.Begin(Old);Check(S.NextNightClock()<0&&S.Ready(Dusk)&&S.WasPhotographed()&&!S.ReadyNight(21.25*3600.)&&S.Update(9*3600.,false)&&S.ReadyNight(S.NextNightClock()),TEXT("An older save keeps its record and gets the coming night"));
 bool Evening=true;for(int32 V=0;V<12;++V){const double H=NightSlot(V)/3600.;Evening&=H>=20&&H+NightWindow/3600.<=24.5&&NightGap(V)>=1;}
 Check(Evening,TEXT("Every night's visit is in the dark and over before their hours"));
 const FVector Anchor(HomeX,2,HomeZ);bool Bounded=true;
 for(int32 I=0;I<Count;++I)for(double T=0;T<Duration;T+=.1)Bounded&=FVector::Dist(Anchor,Position(Anchor,T,I))<17.&&FVector::Dist(Position(Anchor,T,I),Position(Anchor,T+.1,I))<.8;
 Check(Bounded&&Position(Anchor,0).Equals(Anchor),TEXT("Orbs stay around S-02 and S-03 without discontinuous steps"));
 // Every leg after the first is spent at a named thing: close to its hover point, up to the sniffing and the second orb's place beside it.
 bool AtEquipment=true;int32 StopsSeen=0;
 for(int32 I=0;I<Count;++I)for(double T=0;T<Duration;T+=.1)
 {
  const int32 At=StopAt(T,I);if(At<0)continue;++StopsSeen;
  const FStop& E=Stops()[At];const FVector Point=Position(Anchor,T,I);
  AtEquipment&=FVector2D::Distance(FVector2D(Point.X,Point.Z),FVector2D(E.X,E.Z))<2.2;
 }
 Check(AtEquipment&&StopsSeen>Count*100&&StopAt(0,0)<0,TEXT("Orbs stop at the equipment: masts, cabinet, drive box, reflector rims"));
 // Native fixtures and a person's first look stand nine metres south of the first orb: it must not chase them off by moving on.
 bool Clear=true;
 for(double T=1.5;T<25;T+=.1)
 {
  const FVector First=Position(Anchor,T,0);const FVector2D Person(First.X,First.Z+9);
  for(double Later=0;Later<=5;Later+=.2)for(int32 I=0;I<Count;++I)
  {const FVector Other=Position(Anchor,T+Later,I);Clear&=FVector2D::Distance(Person,FVector2D(Other.X,Other.Z))>FleeRange;}
 }
 Check(Clear,TEXT("Nine metres south of the first orb stays outside their flight range through its first stops"));
 Check(!Position(Anchor,60,1).Equals(Position(Anchor,60,2),.5)&&FVector::Dist(Position(Anchor,60),Position(Anchor,70))>.5,TEXT("Orbs investigate separate spots and move on"));
 Check(FleeRise(0)==0&&FleeRise(FleeSeconds)>100.,TEXT("A startled orb is far overhead within its flight time"));
 Check(Visibility(0)==0&&Visibility(10)==1&&Visibility(Duration)==0,TEXT("Orb fades in and out without an abrupt visible teleport"));
 return OK;
}
