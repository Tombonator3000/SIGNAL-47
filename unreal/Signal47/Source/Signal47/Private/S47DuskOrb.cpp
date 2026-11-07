#include "S47DuskOrb.h"

namespace
{
 constexpr double Day=86400.,Dusk=18.5*3600.,DuskEnd=19.25*3600.,Grace=120.;
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
}
void FS47DuskOrb::Begin(const S47J::FObj& Saved)
{
 DueClock=-1;Attempts=0;bPhotographed=false;
 if(!Saved)return;
 const double Due=S47J::Num(Saved,TEXT("dueClock"),-1);
 if(ValidClock(Due))DueClock=Due;
 Attempts=FMath::Clamp(S47J::Int(Saved,TEXT("attempts")),0,1000000);
 bPhotographed=S47J::Bool(Saved,TEXT("photographed"));
}
S47J::FObj FS47DuskOrb::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("dueClock"),DueClock);S47J::Set(O,TEXT("attempts"),Attempts);S47J::Set(O,TEXT("photographed"),bPhotographed);return O;
}
bool FS47DuskOrb::Update(double Clock,bool bTutorial)
{
 if(bTutorial||!ValidClock(Clock))return false;
 if(DueClock<0){DueClock=NextOpportunity(Clock);return true;}
 // A skipped dusk is not a witnessed event. Reschedule without inventing an
 // encounter, evidence, warning or payment, including after a sleep/Continue.
 if(Clock>DayStart(DueClock)+DuskEnd){DueClock=NextOpportunity(Clock)+(bPhotographed?(PhotographedGap(Attempts)-1)*Day:0.);return true;}
 return false;
}
bool FS47DuskOrb::Ready(double Clock) const
{
 return DueClock>=0&&ValidClock(Clock)&&Clock>=DueClock&&Clock<=DayStart(DueClock)+DuskEnd;
}
void FS47DuskOrb::Started(double Clock)
{
 if(!ValidClock(Clock))return;
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
 double Hash(double N){const double V=FMath::Sin(N*12.9898)*43758.5453;return V-FMath::FloorToDouble(V);}
 // The spot one orb investigates next: within the antenna corridor, low above the ground.
 FVector Spot(int32 Stop,int32 Index)
 {
  if(Stop==0)return Index==0?FVector::ZeroVector:FVector(Index==1?-5.:6.,.3*Index,Index==1?3.:-3.);
  const double K=Stop*3.1+Index*17.3;
  return FVector(9.*(2*Hash(K)-1),-.8+3.4*Hash(K+1.7),6.*(2*Hash(K+3.9)-1));
 }
}
FVector FS47DuskOrb::Position(const FVector& Anchor,double Age,int32 Index)
{
 // No camera, player position or random draw enters this function; looking away
 // never repositions it. It holds at a spot, sniffs in small circles, then moves on.
 const double T=FMath::Clamp(FMath::IsFinite(Age)?Age:0.,0.,Duration)+Index*2.3;
 constexpr double Leg=11.,Hold=.6; // of each 11 s leg: 60 % sniffing, 40 % travelling
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
 const FVector Anchor(-3.25,2,-47);bool Bounded=true;
 for(int32 I=0;I<Count;++I)for(double T=0;T<Duration;T+=.1)Bounded&=FVector::Dist(Anchor,Position(Anchor,T,I))<13.&&FVector::Dist(Position(Anchor,T,I),Position(Anchor,T+.1,I))<.8;
 Check(Bounded&&Position(Anchor,0).Equals(Anchor),TEXT("Orbs stay within the antenna corridor without discontinuous steps"));
 Check(!Position(Anchor,60,1).Equals(Position(Anchor,60,2),.5)&&FVector::Dist(Position(Anchor,60),Position(Anchor,70))>.5,TEXT("Orbs investigate separate spots and move on"));
 Check(FleeRise(0)==0&&FleeRise(FleeSeconds)>100.,TEXT("A startled orb is far overhead within its flight time"));
 Check(Visibility(0)==0&&Visibility(10)==1&&Visibility(Duration)==0,TEXT("Orb fades in and out without an abrupt visible teleport"));
 return OK;
}
