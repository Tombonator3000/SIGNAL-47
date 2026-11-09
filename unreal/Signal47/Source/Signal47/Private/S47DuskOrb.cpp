#include "S47DuskOrb.h"

namespace
{
 constexpr double Day=86400.,Dusk=18.5*3600.,DuskEnd=19.25*3600.,Grace=120.,NightWindow=3600.;
 bool ValidClock(double Clock){return FMath::IsFinite(Clock)&&Clock>=0&&Clock<=86400000000.;}
 double DayStart(double Clock){return FMath::FloorToDouble(Clock/Day)*Day;}
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
 double NextNight(double Clock,int32 Visits)
 {
  const double Start=DayStart(Clock);const double Slot=Start+NightSlot(Visits);
  const double Candidate=FMath::Max(Slot,Clock+Grace);
  return Candidate<=Slot+NightWindow?Candidate:Slot+Day;
 }
 // How many come and for how long, written out for the same reason. The first dusk is a single lead with time to photograph it.
 // Seeds in Started: the first dusk's and the first night's leave a watcher nine metres off undisturbed through their first stops (SelfTest).
 int32 DuskLights(int32 Visit){static const int32 N[6]={1,2,1,3,1,2};return N[FMath::Abs(Visit)%6];}
 double DuskSeconds(int32 Visit){static const double S[6]={300,240,300,200,270,220};return S[FMath::Abs(Visit)%6];}
 int32 NightLights(int32 Visit){static const int32 N[8]={2,1,3,1,1,2,3,1};return N[FMath::Abs(Visit)%8];}
 double NightSeconds(int32 Visit){static const double S[7]={300,170,240,130,280,200,150};return S[FMath::Abs(Visit)%7];}
}
int32 FS47DuskOrb::EveningGap(int32 Attempts,bool bAfterPhoto)
{
 // Evenings to the next visit. Written out, not drawn: it has to survive Continue. M7: a light at every dusk was a timetable;
 // every second or third evening until it is on film, then every fourth to seventh.
 static const int32 Before[6]={2,3,2,3,3,2};
 static const int32 After[6]={5,4,7,5,6,4};
 return (bAfterPhoto?After:Before)[FMath::Abs(Attempts)%6];
}
int32 FS47DuskOrb::NightGapFor(int32 Visits,bool bAfterPhoto)
{
 // Seven against the six hours: the same hour and the same wait do not come round together for six weeks. Two to four
 // nights apart, and three more once they are on film.
 static const int32 Gaps[7]={3,2,4,2,3,4,2};
 return Gaps[FMath::Abs(Visits)%7]+(bAfterPhoto?3:0);
}
double FS47DuskOrb::MorningAfter(double Clock)
{
 // Seven o'clock: after a night that ends past midnight that is the same calendar day, after an evening the next one.
 const double Start=DayStart(Clock);
 return Clock<Start+7*3600.?Start+7*3600.:Start+Day+7*3600.;
}
void FS47DuskOrb::LeaveFor(const FString& Kind,const FString& Dish,const FVector& At,double Clock,double Began)
{
 if(Kind.IsEmpty()||Kind.Len()>16||Dish.Len()>16||!ValidClock(Clock)||!FMath::IsFinite(At.X)||!FMath::IsFinite(At.Y)||!FMath::IsFinite(At.Z)||At.GetAbsMax()>1e6)return;
 // A second night at the same place before the first was found adds nothing there: the first is what is found.
 if(Remains.ContainsByPredicate([&](const FLeft& L){return L.Kind==Kind&&L.Dish==Dish&&FVector2D::Distance(FVector2D(L.At.X,L.At.Z),FVector2D(At.X,At.Z))<FindRange;}))return;
 if(Remains.Num()>=MaxLeft)Remains.RemoveAt(0);
 FLeft L;L.Kind=Kind;L.Dish=Dish;L.At=At;L.Clock=Clock;L.Began=ValidClock(Began)&&Began<=Clock?Began:Clock;
 L.From=MorningAfter(Clock);L.Until=L.From+1.5*Day;
 Remains.Add(L);
}
int32 FS47DuskOrb::LeftNear(double Clock,const FVector2D& At) const
{
 if(!ValidClock(Clock))return -1;
 for(int32 I=0;I<Remains.Num();++I){const FLeft& L=Remains[I];if(Clock>=L.From&&Clock<=L.Until&&FVector2D::Distance(FVector2D(L.At.X,L.At.Z),At)<=FindRange)return I;}
 return -1;
}
bool FS47DuskOrb::ExpireLeft(double Clock)
{
 if(!ValidClock(Clock))return false;
 return Remains.RemoveAll([&](const FLeft& L){return Clock>L.Until;})>0;
}
void FS47DuskOrb::Begin(const S47J::FObj& Saved)
{
 DueClock=-1;Attempts=0;bPhotographed=false;NightClock=-1;NightVisits=0;Lights=Count;Seconds=Duration;Seed=0;
 bTold=bVisitAfterPhoto=false;VisitStarted=-1;Remains.Reset();
 if(!Saved)return;
 // Older saves have none of these: not yet said, a visit from before the photograph, nothing left out there.
 bTold=S47J::Bool(Saved,TEXT("told"));bVisitAfterPhoto=S47J::Bool(Saved,TEXT("afterPhoto"));
 const double Began=S47J::Num(Saved,TEXT("visitClock"),-1);if(ValidClock(Began))VisitStarted=Began;
 for(const auto& V:S47J::Arr(Saved,TEXT("left")))
 {
  const S47J::FObj* E=nullptr;if(!V.IsValid()||!V->TryGetObject(E)||!E)continue;
  const auto A=S47J::Nums(*E,TEXT("at"));if(A.Num()!=3)continue;
  const double From=S47J::Num(*E,TEXT("from"),-1),Until=S47J::Num(*E,TEXT("until"),-1);
  const int32 Before=Remains.Num();LeaveFor(S47J::Str(*E,TEXT("kind")),S47J::Str(*E,TEXT("dish")),FVector(A[0],A[1],A[2]),S47J::Num(*E,TEXT("clock"),-1),S47J::Num(*E,TEXT("began"),-1));
  // Its own window as saved, when it is one: Continue does not move it.
  if(Remains.Num()>Before&&ValidClock(From)&&ValidClock(Until)&&Until>=From&&Until-From<=2*Day){Remains.Last().From=From;Remains.Last().Until=Until;}
 }
 // An older save in the middle of a visit keeps three lights for the full time.
 Lights=FMath::Clamp(S47J::Int(Saved,TEXT("lights"),Count),1,Count);Seed=S47J::Int(Saved,TEXT("seed"),0);
 const double SavedSeconds=S47J::Num(Saved,TEXT("seconds"),Duration);Seconds=FMath::IsFinite(SavedSeconds)?FMath::Clamp(SavedSeconds,30.,Duration):Duration;
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
 auto O=S47J::New();S47J::Set(O,TEXT("dueClock"),DueClock);S47J::Set(O,TEXT("attempts"),Attempts);S47J::Set(O,TEXT("photographed"),bPhotographed);S47J::Set(O,TEXT("nightDue"),NightClock);S47J::Set(O,TEXT("nightVisits"),NightVisits);S47J::Set(O,TEXT("lights"),Lights);S47J::Set(O,TEXT("seconds"),Seconds);S47J::Set(O,TEXT("seed"),Seed);
 if(bTold)S47J::Set(O,TEXT("told"),true);if(bVisitAfterPhoto)S47J::Set(O,TEXT("afterPhoto"),true);if(VisitStarted>=0)S47J::Set(O,TEXT("visitClock"),VisitStarted);
 if(!Remains.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> Out;
  for(const FLeft& L:Remains){auto E=S47J::New();S47J::Set(E,TEXT("kind"),L.Kind);S47J::Set(E,TEXT("dish"),L.Dish);S47J::Set(E,TEXT("at"),TArray<double>{L.At.X,L.At.Y,L.At.Z});S47J::Set(E,TEXT("clock"),L.Clock);S47J::Set(E,TEXT("began"),L.Began);S47J::Set(E,TEXT("from"),L.From);S47J::Set(E,TEXT("until"),L.Until);Out.Add(MakeShared<FJsonValueObject>(E));}
  O->SetArrayField(TEXT("left"),Out);
 }
 return O;
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
 if(Clock>DayStart(DueClock)+DuskEnd){DueClock=NextOpportunity(Clock)+(bPhotographed?(EveningGap(Attempts,true)-1)*Day:0.);return true;}
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
 bVisitAfterPhoto=bPhotographed;VisitStarted=Clock;
 if(ReadyNight(Clock))
 {
  Lights=NightLights(NightVisits);Seconds=NightSeconds(NightVisits);Seed=NightVisits*37+76;
  NightVisits=FMath::Min(1000000,NightVisits+1);
  NightClock=DayStart(NightClock)+NightGapFor(NightVisits,bPhotographed)*Day+NightSlot(NightVisits);
  return;
 }
 Lights=DuskLights(Attempts);Seconds=DuskSeconds(Attempts);Seed=Attempts*53+165;
 Attempts=FMath::Min(1000000,Attempts+1);
 DueClock=DayStart(Clock)+EveningGap(Attempts,bPhotographed)*Day+Dusk;
}
void FS47DuskOrb::Photographed(double Clock)
{
 if(bPhotographed)return;
 bPhotographed=true;
 // This visit has already set the next dusk and night by the spacing before: put them back to the spacing from here on,
 // counted from the day of the photograph (an older caller without a clock: the day before the dusk that was due).
 const double From=ValidClock(Clock)?DayStart(Clock):DueClock>=0?DayStart(DueClock)-Day:-1.;
 if(From<0)return;
 if(DueClock>=0)DueClock=FMath::Max(DueClock,From+EveningGap(Attempts,true)*Day+Dusk);
 if(NightClock>=0)NightClock=FMath::Max(NightClock,From+NightGapFor(NightVisits,true)*Day+NightSlot(NightVisits));
}
namespace
{
 // A repeatable 0..1 number: never a draw that Continue could not reproduce.
 double OrbRand(int32 Seed,int32 Index,int64 Slot,uint32 Salt)
 {
  uint32 H=uint32(Seed)*2654435761u^uint32(Index+1)*40503u^uint32(Slot+7)*2246822519u^Salt*3266489917u;
  H^=H>>15;H*=2246822519u;H^=H>>13;H*=3266489917u;H^=H>>16;
  return double(H&0xFFFFFFu)/16777216.;
 }
 double OrbTime(double Age){return FMath::Clamp(FMath::IsFinite(Age)?Age:0.,0.,FS47DuskOrb::Duration);}
 // Which of the stops an orb goes to on this leg. Orb 0 keeps to S-03 and orb 1 to S-02, each round its three in one
 // direction from a start of its own; orb 2 spends its first nine legs at S-02 before it crosses. No leg is too long, and
 // a person watching orb 0 from nine metres south during its first stops is outside the flight range of all of them.
 int32 RouteStop(int32 Leg,int32 Index,int32 Seed)
 {
  static const int32 Rounds[2][3]={{0,2,1},{5,3,4}};
  const int32 At=FMath::Max(Leg,1)-1;
  const int32 Side=Index==0?0:Index==1?1:(At/9)%2==0?1:0;
  return Rounds[Side][(At+int32(OrbRand(Seed,Index,0,11)*3.))%3];
 }
 // The spot one orb investigates on a leg, from its home: leg 0 is where it wakes in the open corridor, the rest are the equipment.
 FVector Spot(int32 Leg,int32 Index,int32 Seed)
 {
  if(Leg==0)return Index==0?FVector::ZeroVector:FVector(Index==1?-5.:6.,.3*Index,Index==1?3.:-3.);
  const auto& S=FS47DuskOrb::Stops()[RouteStop(Leg,Index,Seed)];
  // Two lights at one thing hang side by side, not in one place.
  const FVector Beside=Index==2?FVector(-1.1,.5,.9):FVector::ZeroVector;
  return FVector(S.X-FS47DuskOrb::HomeX,S.Up-FS47DuskOrb::HomeUp,S.Z-FS47DuskOrb::HomeZ)+Beside;
 }
 // Uneven on purpose: mostly a few seconds, sometimes a glance, now and then a long careful look.
 double LegHold(int32 Leg,int32 Index,int32 Seed)
 {
  const double A=OrbRand(Seed,Index,Leg,1),B=OrbRand(Seed,Index,Leg,2);
  if(Leg==0)return Index==0?6.5+3.*B:3.+5.*B;
  return A<.2?1.8+1.4*B:A<.8?4.+5.*B:10.+8.*B;
 }
 double LegTravel(int32 Leg,int32 Index,int32 Seed)
 {return FMath::Clamp((Spot(Leg+1,Index,Seed)-Spot(Leg,Index,Seed)).Size()/8.5,.7,3.);}
 struct FLegNow { int32 Leg=0; double In=0,Hold=0,Travel=0; bool bTravel=false; };
 FLegNow LegAt(double Age,int32 Index,int32 Seed)
 {
  // Every hold and hop is at least 2.5 s together, so a visit of 300 s has about a hundred legs at the most.
  const double T=OrbTime(Age);double Start=0;FLegNow N;
  for(;N.Leg<400;++N.Leg)
  {
   N.Hold=LegHold(N.Leg,Index,Seed);N.Travel=LegTravel(N.Leg,Index,Seed);
   if(T<Start+N.Hold){N.In=T-Start;N.bTravel=false;return N;}
   if(T<Start+N.Hold+N.Travel){N.In=T-Start-N.Hold;N.bTravel=true;return N;}
   Start+=N.Hold+N.Travel;
  }
  return N;
 }
 // The small quick corrections of something holding itself up: a new little offset every third of a second, reached at
 // once, over a slower sway. Never quite still, and never the same as another orb's.
 FVector Trim(double T,int32 Index,int32 Seed)
 {
  const double Slot=.3+.2*OrbRand(Seed,Index,0,21);const double At=T/Slot;const int64 K=int64(FMath::FloorToDouble(At));
  const auto Offset=[&](int64 N){return FVector((OrbRand(Seed,Index,N,22)*2.-1.)*.09,(OrbRand(Seed,Index,N,23)*2.-1.)*.05,(OrbRand(Seed,Index,N,24)*2.-1.)*.09);};
  const FVector Sway(.07*FMath::Sin(T*1.1+Index*2.),.04*FMath::Sin(T*1.7+Index),.07*FMath::Cos(T*.9+Index));
  return FMath::Lerp(Offset(K),Offset(K+1),FMath::SmoothStep(0.,.3,At-double(K)))+Sway;
 }
 // Looking a thing over: a dart to a new place across it, a moment still, the next.
 FVector Scan(const FLegNow& N,int32 Index,int32 Seed)
 {
  static const double Row[6][2]={{0,0},{-1,.1},{-1,-.6},{1,-.6},{1,.2},{0,.4}};
  const double Size=N.Leg==0?.3:FS47DuskOrb::Stops()[RouteStop(N.Leg,Index,Seed)].Scan;
  const double Slot=.8+.6*OrbRand(Seed,Index,N.Leg,3);const double At=N.In/Slot;const int64 K=int64(FMath::FloorToDouble(At));
  const int32 From=int32(OrbRand(Seed,Index,N.Leg,4)*6.)+int32(K%6);
  const double Dart=FMath::SmoothStep(.75,1.,At-double(K));
  const double X=FMath::Lerp(Row[From%6][0],Row[(From+1)%6][0],Dart),Y=FMath::Lerp(Row[From%6][1],Row[(From+1)%6][1],Dart);
  const double Turn=OrbRand(Seed,Index,N.Leg,5)*.9-.45;
  const double Fade=FMath::Min(N.In/.35,1.)*FMath::Min(FMath::Max(N.Hold-N.In,0.)/.3,1.);
  return (FVector(FMath::Cos(Turn),0,FMath::Sin(Turn))*X+FVector(0,Y,0))*Size*Fade;
 }
}
TArrayView<const FS47DuskOrb::FStop> FS47DuskOrb::Stops()
{
 // Beside the mast under the dish, at the door of the box, under the low rim of the reflector at its usual az 026, el 32.
 static const FStop Data[6]={
  {TEXT("mast:S-03"),TEXT("S-03 mast"),6.4,-43.6,2.6,.45},
  {TEXT("field:S-03"),TEXT("S-03 junction cabinet"),4.9,-41.,1.4,.28},
  {TEXT("mast:S-03"),TEXT("S-03 reflector rim"),12.4,-47.9,4.,.3},
  {TEXT("mast:S-02"),TEXT("S-02 mast"),-13.,-49.7,2.6,.45},
  {TEXT("field:S-02"),TEXT("S-02 drive box"),-10.1,-54.6,1.5,.35},
  {TEXT("mast:S-02"),TEXT("S-02 reflector rim"),-13.2,-57.7,4.,.5}};
 return MakeArrayView(Data);
}
int32 FS47DuskOrb::StopAt(double Age,int32 Index,int32 Seed)
{
 const FLegNow N=LegAt(Age,Index,Seed);
 return N.Leg<1||N.bTravel?-1:RouteStop(N.Leg,Index,Seed);
}
FVector FS47DuskOrb::Position(const FVector& Anchor,double Age,int32 Index,int32 Seed)
{
 // No camera, player position or random draw enters this function; looking away never repositions it.
 const double T=OrbTime(Age);const FLegNow N=LegAt(T,Index,Seed);
 const FVector Here=Spot(N.Leg,Index,Seed);
 FVector At=Here;
 if(N.bTravel)
 {
  // A hop is one quick banked curve: off, across and on to the next thing, never a straight slide.
  const FVector To=Spot(N.Leg+1,Index,Seed);const double U=FMath::Clamp(N.In/N.Travel,0.,1.);
  const FVector Way=To-Here;const FVector Side=FVector(-Way.Z,0,Way.X).GetSafeNormal();
  const double Bank=FMath::Min(Way.Size()*.12,1.2)*(OrbRand(Seed,Index,N.Leg,6)<.5?-1.:1.);
  At=FMath::Lerp(Here,To,FMath::SmoothStep(0.,1.,U))+(Side*Bank+FVector(0,.3,0))*FMath::Sin(U*UE_DOUBLE_PI);
 }
 else
 {
  At+=Scan(N,Index,Seed);
  // It stops a little past the place and comes back to it, and it sets itself before going: the lean of a thing under rotors.
  if(N.Leg>=1)At+=(Here-Spot(N.Leg-1,Index,Seed)).GetSafeNormal()*(.3*FMath::Exp(-N.In*4.5)*FMath::Sin(N.In*9.));
  const double Set=FMath::SmoothStep(N.Hold-.45,N.Hold-.15,N.In)*(1.-FMath::SmoothStep(N.Hold-.15,N.Hold,N.In));
  At-=(Spot(N.Leg+1,Index,Seed)-Here).GetSafeNormal()*(.12*Set);
 }
 // The corrections fade in from nothing so the first orb starts exactly on its anchor.
 return Anchor+At+Trim(T,Index,Seed)*FMath::Min(T/4.,1.);
}
FVector FS47DuskOrb::LightOffset(double Age,int32 Index,int32 Seed)
{
 const FVector Velocity=(Position(FVector::ZeroVector,Age+.04,Index,Seed)-Position(FVector::ZeroVector,Age-.04,Index,Seed))*(1./.08);
 return FVector(0,-.25-.45*ScanLevel(Age,Index,Seed),0)-(Velocity*.04).GetClampedToMaxSize(.7);
}
double FS47DuskOrb::ScanLevel(double Age,int32 Index,int32 Seed)
{
 const FLegNow N=LegAt(Age,Index,Seed);
 if(N.Leg<1||N.bTravel)return 0;
 return FMath::SmoothStep(.2,.7,N.In)*(1.-FMath::SmoothStep(N.Hold-.5,N.Hold-.1,N.In));
}
double FS47DuskOrb::Presence(double Age,int32 Index,int32 Seed,double VisitLength)
{
 const double Whole=Visibility(Age,VisitLength);
 if(Index==0||!FMath::IsFinite(Age))return Whole;
 const double Comes=6.+26.*OrbRand(Seed,Index,0,31),Goes=VisitLength-8.-40.*OrbRand(Seed,Index,0,32);
 return Whole*FMath::Clamp((Age-Comes)/2.,0.,1.)*FMath::Clamp((Goes-Age)/2.,0.,1.);
}
FS47DuskOrb::FFlight FS47DuskOrb::Flight(int32 Index,int32 Seed,double VisitLength)
{
 FFlight F;F.ShortestHold=1e9;double Start=0;FVector Was=Position(FVector::ZeroVector,0,Index,Seed);
 for(int32 Leg=0;Leg<400&&Start<VisitLength;++Leg)
 {
  const double Hold=LegHold(Leg,Index,Seed),Travel=LegTravel(Leg,Index,Seed);
  if(Leg>=1){F.ShortestHold=FMath::Min(F.ShortestHold,Hold);F.LongestHold=FMath::Max(F.LongestHold,Hold);}
  if(Start+Hold<VisitLength){++F.Hops;if(F.Hops==1)F.FirstHopAt=Start+Hold;F.LongestHop=FMath::Max(F.LongestHop,(Spot(Leg+1,Index,Seed)-Spot(Leg,Index,Seed)).Size());}
  Start+=Hold+Travel;
 }
 for(double T=.05;T<VisitLength;T+=.05)
 {const FVector Now=Position(FVector::ZeroVector,T,Index,Seed);F.Fastest=FMath::Max(F.Fastest,FVector::Dist(Now,Was)/.05);Was=Now;}
 return F;
}
double FS47DuskOrb::FleeRise(double FleeAge)
{
 const double T=FMath::Max(0.,FMath::IsFinite(FleeAge)?FleeAge:0.);
 return .5*150.*T*T;
}
double FS47DuskOrb::Visibility(double Age,double VisitLength)
{
 if(!FMath::IsFinite(Age))return 0;
 return FMath::Min(FMath::Clamp(Age/3.,0.,1.),FMath::Clamp((VisitLength-Age)/8.,0.,1.));
}
bool FS47DuskOrb::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Pass,const TCHAR* Text){Lines.Add(FString(Pass?TEXT("PASS "):TEXT("FAIL "))+Text);OK&=Pass;};
 FS47DuskOrb S;S.Begin(nullptr);
 Check(!S.Update(8*3600.,true)&&!S.Ready(Dusk),TEXT("Dusk orb does not schedule during tutorial"));
 Check(S.Update(9*3600.,false)&&!S.Ready(Dusk-1)&&S.Ready(Dusk),TEXT("Early tutorial completion schedules the coming dusk"));
 S.Started(Dusk);FS47DuskOrb R;R.Begin(S47J::Read(S47J::Write(S.Save())));
 Check(!R.Ready(Dusk)&&!R.Ready(Day+Dusk)&&(R.Ready(2*Day+Dusk)||R.Ready(3*Day+Dusk)),TEXT("Consumed dusk survives Continue without restarting or losing next opportunity, two or three evenings on"));
 R.Photographed(Dusk+60);S.Begin(R.Save());Check(!S.Ready(Day+Dusk)&&!S.Ready(2*Day+Dusk)&&!S.Ready(3*Day+Dusk)&&(S.Ready(4*Day+Dusk)||S.Ready(5*Day+Dusk)||S.Ready(6*Day+Dusk)||S.Ready(7*Day+Dusk))&&S.WasPhotographed(),TEXT("After a photograph the lights come back every fourth to seventh evening; the record of it survives Continue"));
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
 bool Evening=true;for(int32 V=0;V<12;++V){const double H=NightSlot(V)/3600.;Evening&=H>=20&&H+NightWindow/3600.<=24.5&&NightGapFor(V,false)>=1;}
 Check(Evening,TEXT("Every night's visit is in the dark and over before their hours"));
 // Each visit's own count, length and seed, and they survive Continue.
 FS47DuskOrb V;V.Begin(nullptr);V.Update(9*3600.,false);bool SawLights[4]={};bool LightsOk=true;double ShortestVisit=1e9,LongestVisit=0;int32 LastLights=0,Same=0;TArray<double> Waits;double LastDay=-1;
 for(int32 K=0;K<42;++K)
 {
  const double At=V.NextNightClock();V.Started(At);LightsOk&=V.VisitLights()>=1&&V.VisitLights()<=Count&&V.VisitSeconds()<=Duration&&V.VisitSeconds()>=30;
  SawLights[V.VisitLights()]=true;ShortestVisit=FMath::Min(ShortestVisit,V.VisitSeconds());LongestVisit=FMath::Max(LongestVisit,V.VisitSeconds());
  if(V.VisitLights()==LastLights)++Same;LastLights=V.VisitLights();
  const double Today=FMath::FloorToDouble(At/Day);if(LastDay>=0)Waits.Add(Today-LastDay);LastDay=Today;
 }
 bool WaitsDiffer=false;for(int32 K=1;K<Waits.Num();++K)WaitsDiffer|=Waits[K]!=Waits[0];
 Check(LightsOk&&SawLights[1]&&SawLights[2]&&SawLights[3]&&LongestVisit>ShortestVisit*1.5&&WaitsDiffer&&Same<30,TEXT("Night visits are sporadic: one to three lights, of uneven length, nights apart unevenly"));
 FS47DuskOrb VC;VC.Begin(S47J::Read(S47J::Write(V.Save())));
 Check(VC.VisitLights()==V.VisitLights()&&VC.VisitSeconds()==V.VisitSeconds()&&VC.VisitSeed()==V.VisitSeed(),TEXT("A visit's lights, length and flight survive Continue"));
 FS47DuskOrb Dk;Dk.Begin(nullptr);Dk.Update(9*3600.,false);Dk.Started(Dusk);
 Check(Dk.VisitLights()==1&&Dk.VisitSeconds()==Duration,TEXT("The first dusk is one light with the whole visit to photograph it"));
 const FVector Anchor(HomeX,2,HomeZ);bool Bounded=true,AtEquipment=true,Started=true,Quick=true,Uneven=true,Trimmed=true,Patient=true;int32 StopsSeen=0,Own=0;double WorstStep=0;
 for(int32 Seed=0;Seed<48;++Seed)
 {
  Started&=Position(Anchor,0,0,Seed).Equals(Anchor);
  for(int32 I=0;I<Count;++I)
  {
   for(double T=0;T<Duration;T+=.1)
   {
    const FVector P=Position(Anchor,T,I,Seed);const double Step=FVector::Dist(P,Position(Anchor,T+.1,I,Seed));
    Bounded&=FVector::Dist(Anchor,P)<18.5;WorstStep=FMath::Max(WorstStep,Step);
    // Every leg after the first is spent at a named thing: close to its hover point, up to the darts and the second orb's place beside it.
    const int32 At=StopAt(T,I,Seed);if(At<0)continue;++StopsSeen;
    const FStop& E=Stops()[At];AtEquipment&=FVector2D::Distance(FVector2D(P.X,P.Z),FVector2D(E.X,E.Z))<2.6;
   }
   const FFlight F=Flight(I,Seed);
   Quick&=F.Hops>=8&&F.Fastest<20.&&F.Fastest>4.;Uneven&=F.LongestHold>F.ShortestHold*1.6;
   // Hovering is not standing: at a stop it moves a little within half a second, and it never leaves the thing it looks at.
   double Moved=0;for(double T=20;T<Duration;T+=.25)if(StopAt(T,I,Seed)>=0&&StopAt(T+.25,I,Seed)>=0)Moved=FMath::Max(Moved,FVector::Dist(Position(Anchor,T,I,Seed),Position(Anchor,T+.25,I,Seed)));
   Trimmed&=Moved>.05&&Moved<1.5;
  }
  Own+=FMath::Abs(Flight(0,Seed).FirstHopAt-Flight(1,Seed).FirstHopAt)>.05&&!Position(Anchor,60,1,Seed).Equals(Position(Anchor,60,2,Seed),.5)?1:0;
  // Someone standing nine metres off is left alone for as long as it stays at the thing it is looking at, on any visit.
  for(double T=1.5;T<25;T+=.25)
  {
   const int32 Holding=StopAt(T,0,Seed);if(Holding<0)continue;
   const FVector First=Position(Anchor,T,0,Seed);const FVector2D Person(First.X,First.Z+9);
   for(double Later=0;Later<=5&&StopAt(T+Later,0,Seed)==Holding;Later+=.25)for(int32 I=0;I<Count;++I)
   {const FVector Other=Position(Anchor,T+Later,I,Seed);Patient&=FVector2D::Distance(Person,FVector2D(Other.X,Other.Z))>FleeRange;}
  }
 }
 // Native fixtures and a person's first look stand nine metres south of the first orb on the first dusk and the first night: it must not chase them off by moving on.
 bool Clear=true;FS47DuskOrb First;First.Begin(nullptr);First.Update(9*3600.,false);First.Started(Dusk);const int32 DuskSeed=First.VisitSeed();First.Started(First.NextNightClock());const int32 NightSeed=First.VisitSeed();
 for(const int32 Seed:{DuskSeed,NightSeed})for(double T=1.5;T<25;T+=.1)
 {
  const FVector Lead=Position(Anchor,T,0,Seed);const FVector2D Person(Lead.X,Lead.Z+9);
  for(double Later=0;Later<=5;Later+=.2)for(int32 I=0;I<Count;++I)
  {const FVector Other=Position(Anchor,T+Later,I,Seed);Clear&=FVector2D::Distance(Person,FVector2D(Other.X,Other.Z))>FleeRange;}
 }
 Check(Bounded&&Started&&WorstStep<2.,TEXT("Orbs stay around S-02 and S-03 without discontinuous steps, on every seed"));
 Check(AtEquipment&&StopsSeen>Count*100&&StopAt(0,0,0)<0,TEXT("Orbs stop at the equipment: masts, cabinet, drive box, reflector rims"));
 Check(Clear&&Patient&&DuskSeed!=NightSeed,TEXT("Nine metres south of the first orb stays outside their flight range through its first stops"));
 Check(Quick&&Uneven&&Trimmed,TEXT("Flight as drones: hovers with small corrections, stops unevenly long, hops quickly"));
 Check(Own>=42&&FVector::Dist(Position(Anchor,60,0,3),Position(Anchor,70,0,3))>.5&&!Position(Anchor,60,0,3).Equals(Position(Anchor,60,0,4),.05),TEXT("Each orb flies for itself, and no two visits fly alike"));
 bool Fades=true;for(int32 Seed=0;Seed<12;++Seed)for(int32 I=1;I<Count;++I)Fades&=Presence(0,I,Seed)==0&&Presence(150,I,Seed)==1&&Presence(Duration,I,Seed)==0&&Presence(5.5,I,Seed)==0;
 Check(Fades&&Presence(10,0,0)==1,TEXT("The others come later and go earlier than the first, each by a fade"));
 Check(ScanLevel(0)==0&&!LightOffset(30,1,2).IsNearlyZero(),TEXT("The light hangs under its orb and dips as it looks"));
 Check(FleeRise(0)==0&&FleeRise(FleeSeconds)>100.,TEXT("A startled orb is far overhead within its flight time"));
 Check(Visibility(0)==0&&Visibility(10)==1&&Visibility(Duration)==0&&Visibility(130,130)==0&&Visibility(100,130)==1,TEXT("Orb fades in and out without an abrupt visible teleport, whatever the visit's length"));
 // M7 (B2): rarer, and spaced further once on film; dusk and night both, through the real schedule.
 {
  FS47DuskOrb G;G.Begin(nullptr);G.Update(9*3600.,false);bool BeforeOk=true,AfterOk=true,NightsOk=true,LaterOk=true,FlagOk=true;double Was=-1;int32 Visits=0;
  for(int32 K=0;K<10;++K){const double At=G.DueClock;G.Started(At);FlagOk&=!G.VisitAfterPhoto()&&G.VisitClock()==At;++Visits;if(Was>=0){const double Gap=(DayStart(At)-DayStart(Was))/Day;BeforeOk&=Gap>=2&&Gap<=3;}Was=At;}
  G.Photographed(Was+60);
  for(int32 K=0;K<10;++K){const double At=G.DueClock;const double Gap=(DayStart(At)-DayStart(Was))/Day;AfterOk&=Gap>=4&&Gap<=7;G.Started(At);FlagOk&=G.VisitAfterPhoto();Was=At;}
  for(int32 Ng=0;Ng<14;++Ng){NightsOk&=NightGapFor(Ng,false)>=2&&NightGapFor(Ng,false)<=4;LaterOk&=NightGapFor(Ng,true)>=5&&NightGapFor(Ng,true)<=7;}
  // Thirty days before any photograph: dusk and night together, about half of what came before (forty-odd).
  FS47DuskOrb T;T.Begin(nullptr);T.Update(9*3600.,false);int32 Month=0;for(int32 K=0;K<200;++K){const double Next=FMath::Min(T.DueClock,T.NightClock);if(Next>30*Day)break;T.Started(Next);++Month;}
  Check(BeforeOk&&AfterOk&&FlagOk,TEXT("Before the first photograph every second or third evening, after it every fourth to seventh, and each visit knows which it is"));
  Check(NightsOk&&LaterOk&&Month>=12&&Month<=26,*FString::Printf(TEXT("Nights two to four apart, five to seven once on film; %d visits in a month before any photograph"),Month));
  FS47DuskOrb P;P.Begin(nullptr);P.Update(9*3600.,false);const double Night0=P.NightClock;P.Started(Night0);const double NightNext=P.NightClock;P.Photographed(Night0+60);
  Check(P.NightClock>NightNext&&(DayStart(P.NightClock)-DayStart(Night0))/Day>=5,TEXT("A photograph in the dark puts the next night back as well"));
 }
 {
  FS47DuskOrb Y;Y.Begin(nullptr);Check(!Y.Told(),TEXT("Not yet said in a new game"));Y.Tell();FS47DuskOrb YC;YC.Begin(S47J::Read(S47J::Write(Y.Save())));
  FS47DuskOrb Fresh;Fresh.Begin(S47J::Read(S47J::Write(FS47DuskOrb().Save())));
  Check(YC.Told()&&!Fresh.Told(),TEXT("The one line about them is said once and stays said across Continue; an older save has not said it"));
 }
 {
  // Left at the antenna at 20:40 on day 3 (calendar day 2): nothing that night, there the next morning, gone after a day and a half.
  const double Eve=2*Day+20.67*3600.;const FVector Cab(-40,1.2,-60);
  FS47DuskOrb L;L.Begin(nullptr);L.LeaveFor(TEXT("orb"),TEXT("S-07"),Cab,Eve,Eve-1200);
  const FVector2D On(Cab.X+2,Cab.Z+1),Off(Cab.X+FindRange+1.5,Cab.Z);
  Check(L.LeftNear(Eve+3600,On)<0&&L.LeftNear(3*Day+6.9*3600.,On)<0&&L.LeftNear(3*Day+9*3600.,On)==0&&L.LeftNear(3*Day+9*3600.,Off)<0,TEXT("What a visit leaves is not there the same night, is there from the next morning, and only at the place"));
  L.LeaveFor(TEXT("orb"),TEXT("S-07"),Cab+FVector(1,0,1),Eve+Day);
  FS47DuskOrb LC;LC.Begin(S47J::Read(S47J::Write(L.Save())));
  Check(L.Left().Num()==1&&LC.Left().Num()==1&&LC.Left()[0].Dish==TEXT("S-07")&&LC.Left()[0].Began==Eve-1200&&LC.Left()[0].From==L.Left()[0].From&&LC.LeftNear(3*Day+9*3600.,On)==0,TEXT("A second night at the same place adds nothing there, and what is left survives Continue with its own hours"));
  // After a figure past two in the morning, the same calendar day's morning.
  FS47DuskOrb F;F.Begin(nullptr);F.LeaveFor(TEXT("fence"),FString(),FVector(11.9,1,-11),4*Day+2.5*3600.);
  Check(F.LeftNear(4*Day+3*3600.,FVector2D(11.9,-11))<0&&F.LeftNear(4*Day+8*3600.,FVector2D(11.9,-11))==0,TEXT("After a night past midnight it is there that same morning"));
  Check(!LC.ExpireLeft(3*Day+20*3600.)&&LC.ExpireLeft(5*Day)&&LC.Left().IsEmpty(),TEXT("Not found within a day and a half, it is gone"));
  FS47DuskOrb Many;Many.Begin(nullptr);for(int32 I=0;I<MaxLeft+3;++I)Many.LeaveFor(TEXT("gate"),FString(),FVector(I*20.,1,0),Eve+I*Day);
  bool Bad=true;auto Junk=S47J::New();S47J::Set(Junk,TEXT("left"),TArray<double>{1,2,3});FS47DuskOrb J;J.Begin(Junk);Bad&=J.Left().IsEmpty();
  Many.Found(0);Check(Many.Left().Num()==MaxLeft-1&&Bad,TEXT("At most a few wait at once, a found one is gone, and a damaged save invents none"));
 }
 return OK;
}
