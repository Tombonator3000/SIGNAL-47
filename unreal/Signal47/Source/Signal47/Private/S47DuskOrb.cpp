#include "S47DuskOrb.h"

namespace
{
 constexpr double Day=86400.,Dusk=18.5*3600.,DuskEnd=19.25*3600.,Grace=120.;
 bool ValidClock(double Clock){return FMath::IsFinite(Clock)&&Clock>=0&&Clock<=86400000000.;}
 double DayStart(double Clock){return FMath::FloorToDouble(Clock/Day)*Day;}
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
 if(bTutorial||bPhotographed||!ValidClock(Clock))return false;
 if(DueClock<0){DueClock=NextOpportunity(Clock);return true;}
 // A skipped dusk is not a witnessed event. Reschedule without inventing an
 // encounter, evidence, warning or payment, including after a sleep/Continue.
 if(Clock>DayStart(DueClock)+DuskEnd){DueClock=NextOpportunity(Clock);return true;}
 return false;
}
bool FS47DuskOrb::Ready(double Clock) const
{
 return !bPhotographed&&DueClock>=0&&ValidClock(Clock)&&Clock>=DueClock&&Clock<=DayStart(DueClock)+DuskEnd;
}
void FS47DuskOrb::Started(double Clock)
{
 if(!ValidClock(Clock))return;
 Attempts=FMath::Min(1000000,Attempts+1);
 DueClock=DayStart(Clock)+Day+Dusk;
}
FVector FS47DuskOrb::Position(const FVector& Anchor,double Age)
{
 // One calm, continuous world-space patrol. No camera, player position or
 // random draw enters this function; looking away never repositions it.
 const double T=FMath::Clamp(FMath::IsFinite(Age)?Age:0.,0.,Duration);
 const double Phase=T*(2.*PI/180.);
 return Anchor+FVector(7.*FMath::Sin(Phase),.18*FMath::Sin(Phase*.5),2.5*FMath::Sin(Phase));
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
 R.Photographed();S.Begin(R.Save());Check(!S.Ready(Day+Dusk)&&!S.Update(3*Day,false),TEXT("Retained photograph ends invitation repeats across Continue"));
 S.Begin(nullptr);S.Update(22*3600.,false);Check(!S.Ready(22*3600.)&&S.Ready(Day+Dusk),TEXT("Late tutorial completion waits for the next dusk"));
 S.Begin(nullptr);S.Update(18.75*3600.,false);Check(!S.Ready(18.75*3600.)&&S.Ready(18.75*3600.+Grace),TEXT("Tutorial completed at dusk gets a short saved grace period"));
 S.Begin(nullptr);S.Update(8*3600.,false);Check(S.Update(Day+8*3600.,false)&&!S.Ready(Day+8*3600.)&&S.Ready(Day+Dusk),TEXT("Sleeping past dusk reschedules without manufacturing an encounter"));
 const FVector Anchor(-3.25,2,-47);bool Bounded=true;
 for(double T=0;T<Duration;T+=.1)Bounded&=FVector::Dist(Anchor,Position(Anchor,T))<8.&&FVector::Dist(Position(Anchor,T),Position(Anchor,T+.1))<.03;
 Check(Bounded&&Position(Anchor,0).Equals(Anchor),TEXT("Orb patrol remains within the antenna corridor without discontinuous steps"));
 Check(Visibility(0)==0&&Visibility(10)==1&&Visibility(Duration)==0,TEXT("Orb fades in and out without an abrupt visible teleport"));
 return OK;
}
