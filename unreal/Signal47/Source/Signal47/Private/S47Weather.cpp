#include "S47Weather.h"
#include "S47Omen.h"
#include "S47Sky.h"

// File-local helpers live in a named namespace: in a unity build an anonymous one would share its names with every other file in it.
namespace S47WeatherLocal
{
 // A stable draw in [0,1) from the seed, the day and a salt: no global state, nothing that depends on frames.
 double Draw(int32 Seed,int32 OnDay,uint32 Salt)
 {
  uint32 Hash=uint32(Seed)*2654435761u^uint32(OnDay)*2246822519u^(Salt+1u)*3266489917u;
  Hash^=Hash>>15;Hash*=2246822519u;Hash^=Hash>>13;Hash*=3266489917u;Hash^=Hash>>16;
  return double(Hash)/4294967296.;
 }
 // The number the game keeps out of everything said in Freeform never comes up in a paper either.
 int32 Clean(int32 V){return V==47?46:V;}
 constexpr double Synodic=29.530588853,EpochJd=2451550.26; // a new moon: 6 January 2000, 18:14 UT
 // Where the sky is read for the paper's morning, afternoon and night (FS47Sky::CloudCoverAt takes the clock).
 constexpr double Morning=9.5*3600.,Afternoon=15*3600.,Night=23*3600.;
 // FS47Ambience: 16 C at nine in the evening, a degree and a fifth colder every hour, about 5 C (41 F) by six in the morning.
 constexpr int32 DawnF=41;
 const TCHAR* SkyWord(float Cover){return Cover<.25f?TEXT("clear"):Cover<.47f?TEXT("mostly sunny"):Cover<.62f?TEXT("partly cloudy"):TEXT("mostly cloudy");}
 FString Capital(const FString& S){FString Out=S;if(!Out.IsEmpty())Out[0]=FChar::ToUpper(Out[0]);return Out;}
 double Smooth01(double X){X=FMath::Clamp(X,0.,1.);return X*X*(3.-2.*X);}
 // Up from A to B, held, down from C to D (clock hours).
 double Trap(double H,double A,double B,double C,double D){return Smooth01((H-A)/FMath::Max(.01,B-A))*(1.-Smooth01((H-C)/FMath::Max(.01,D-C)));}
 // A windy spell runs about two days (spring fronts), on a little over a quarter of the pairs of days; never the first two days, so
 // that the station is learnt in plain weather.
 bool Spell(int32 Seed,int32 Num){return Num>2&&Draw(Seed,Num/2,11)<.28;}
 // A stray shower: about one day in nine of the calm ones, from the third day.
 bool ShowerOn(int32 Seed,int32 Num){return Num>=3&&!Spell(Seed,Num)&&Draw(Seed,Num,19)<.11;}
 // What the day's wind is at a clock hour: still at dawn, up by late morning, its most in the afternoon, down at dusk; on a windy day
 // some of it holds into the night as NightStorm says, gone by about one. A stormy night (NightStorm over a half, FS47Rhythm's busier
 // night: M5 critic 3) holds it through the small hours, when the night's own visitors are about, easing from four and gone by six.
 // H runs on past midnight (24 to 30) for the night after the day.
 constexpr double StillMph=4; // the night's and the dawn's own breeze, every day the same, so that midnight is no seam
 constexpr float HeldNight=.5f; // NightStorm above this: the wind holds into the small hours
 constexpr double SmallHours=6; // how far into the next day the night before's wind and sand can reach (FS47NightRhythm::EndHour)
 double MphAt(const S47Weather::FDay& D,double H)
 {
  double Mph=StillMph+(D.WindMph-StillMph)*Trap(H,8.5,13.,17.5,21.);
  if(D.NightStorm>HeldNight&&H>=17)Mph=FMath::Max(Mph,D.WindMph*(.5+.35*D.NightStorm)*(1.-Smooth01((H-28.)/2.)));
  else if(D.NightStorm>0&&H>=17)Mph=FMath::Max(Mph,D.WindMph*.7*D.NightStorm*(1.-Smooth01((H-23.)/2.)));
  return Mph;
 }
 // The sand carried on with the wind after dark: gone by midnight, or on a stormy night by about half past three.
 double NightDust(const S47Weather::FDay& D,double H)
 {
  if(!D.bDust||H<D.DustTo-1.)return 0.;
  const double End=D.NightStorm>HeldNight?26.:22.5;
  return .4*D.DustK*D.NightStorm*(1.-Smooth01((H-End)/1.5));
 }
 // The way the wind blows from, as the gusts in FS47Omen do.
 FString WayFrom(const S47Weather::FDay& D){return FS47Omen::Way(FMath::DegreesToRadians(double(D.WindFrom)));}
}

S47Weather::FDay S47Weather::Day(int32 Seed,int32 ShiftDay)
{
 namespace L=S47WeatherLocal;
 FDay D;const int32 Num=FMath::Max(1,ShiftDay);
 // The warmth, which nobody sees: a slow climb through the spring (about 76 F at the end of April, 85 F at the end of May), spells
 // of three days or so that run warmer or colder, and the day's own bit. The night comes down to the ambience's dawn.
 const double Trend=76.+.3*(Num-1),Spell=(L::Draw(Seed,Num/3,2)-.5)*8.,Own=(L::Draw(Seed,Num,3)-.5)*4.;
 D.HighF=FMath::Clamp(FMath::RoundToInt(Trend+Spell+Own),68,92);
 D.LowF=L::DawnF-1+FMath::FloorToInt(L::Draw(Seed,Num,5)*3.);
 D.WindFrom=float(FMath::RadiansToDegrees(FS47Omen::WindFrom(Seed,Num)));
 if(L::Spell(Seed,Num))
 {
  // A strong south-west wind in the afternoon; on most such days it lifts the sand.
  D.WindMph=20+FMath::FloorToInt(L::Draw(Seed,Num,12)*12.);
  D.GustMph=D.WindMph+12+FMath::FloorToInt(L::Draw(Seed,Num,13)*10.);
  if(D.WindMph>=23&&L::Draw(Seed,Num,14)<.78)
  {
   D.bDust=true;D.DustK=float(.45+.55*L::Draw(Seed,Num,15));
   D.DustFrom=float(12.5+2.*L::Draw(Seed,Num,16));D.DustTo=float(18.5+1.5*L::Draw(Seed,Num,17));
   D.NightStorm=float(.35+.65*L::Draw(Seed,Num,18));
  }
  else D.NightStorm=0.f; // (a windy day without sand is still by night: a stormy night is a rare one, and a felt one, M5 critic 3)
 }
 else
 {
  // The light wind the brush sways in (S47Wind) and the ambience's gusts now and then.
  D.WindMph=6+FMath::FloorToInt(L::Draw(Seed,Num,7)*6.);
  D.GustMph=D.WindMph+9+FMath::FloorToInt(L::Draw(Seed,Num,8)*6.);
 }
 if(L::ShowerOn(Seed,Num))
 {
  D.bShower=true;D.ShowerAt=float(15.+4.5*L::Draw(Seed,Num,20));D.bThunder=L::Draw(Seed,Num,21)<.55;
  D.HighF-=4; // a cooler afternoon under the cloud
 }
 // Haze in the morning: always after a shower, now and then on a still morning.
 if(!D.Windy()&&((Num>1&&L::ShowerOn(Seed,Num-1))||L::Draw(Seed,Num,22)<.15)){D.bHaze=true;D.HazeK=float(.5+.5*L::Draw(Seed,Num,23));}
 D.bLightning=true;
 // The sky's own cover with what the day adds to it (a shower's cloud, the sand in place of cloud, the haze).
 const auto Cover=[&D](double Clock){return float(FMath::Clamp(FS47Sky::CloudCoverAt(Clock)+CloudBiasAt(D,Clock),0.,.9));};
 D.CloudAm=Cover(L::Morning);D.CloudPm=Cover(L::Afternoon);D.CloudNight=Cover(L::Night);
 D.HighF=L::Clean(D.HighF);D.LowF=L::Clean(D.LowF);D.WindMph=L::Clean(D.WindMph);D.GustMph=L::Clean(D.GustMph);
 return D;
}
double S47Weather::CloudBiasAt(const FDay& D,double ClockOfDay)
{
 namespace L=S47WeatherLocal;
 const double H=FMath::Fmod(FMath::Fmod(ClockOfDay,86400.)+86400.,86400.)/3600.;
 double B=0;
 // a shower's cloud builds through the afternoon and goes after it; on a sand day the brown takes the place of the cloud
 if(D.bShower){double Rain,Wet,Cloud;bool bThunder;ShowerCourse(H-D.ShowerAt,false,Rain,Wet,Cloud,bThunder);B+=Cloud;}
 if(D.bDust)B-=.12*L::Trap(H,D.DustFrom-1.,D.DustFrom+.5,D.DustTo-1.,D.DustTo+1.);
 if(D.bHaze)B+=.06*L::Trap(H,4.,5.,8.5,10.5);
 return B;
}
void S47Weather::ShowerCourse(double Hours,bool bThunder,double& OutRain,double& OutWet,double& OutCloud,bool& bOutThunder)
{
 namespace L=S47WeatherLocal;
 const double H=FMath::IsFinite(Hours)?Hours:NoShower;
 // (M5 critic 3: long enough to be seen and heard, about three quarters of an hour at its fullest)
 OutRain=L::Trap(H,-.25,.05,.8,1.1);
 OutWet=L::Trap(H,0.,.35,1.5,4.);
 // its cloud builds through the afternoon and goes after it
 OutCloud=.24*L::Trap(H,-3.,-.5,1.5,3.5);
 bOutThunder=bThunder&&L::Trap(H,-1.,-.5,1.,1.6)>0;
}
S47Weather::FNow S47Weather::Now(int32 Seed,int32 CalendarDay,double ClockOfDay)
{
 namespace L=S47WeatherLocal;
 const int32 Num=FMath::Max(1,CalendarDay);
 const FDay D=Day(Seed,Num);
 const double H=FMath::Fmod(FMath::Fmod(ClockOfDay,86400.)+86400.,86400.)/3600.;
 FNow N;
 double Mph=L::MphAt(D,H);
 // the small hours have what is left of the night before's wind
 const bool bAfter=H<L::SmallHours&&Num>1;
 const FDay Before=bAfter?Day(Seed,Num-1):FDay();
 if(bAfter)Mph=FMath::Max(Mph,L::MphAt(Before,H+24.));
 N.Mph=Mph;N.Wind=FMath::Clamp((Mph-12.)/22.,0.,1.);
 if(D.bDust)
 {
  N.Dust=D.DustK*L::Trap(H,D.DustFrom,D.DustFrom+1.2,D.DustTo-1.,D.DustTo+.6);
  // and some of it carried on with the wind after dark
  N.Dust=FMath::Max(N.Dust,L::NightDust(D,H));
 }
 // (a stormy night's sand still in the air in the small hours)
 if(bAfter)N.Dust=FMath::Max(N.Dust,L::NightDust(Before,H+24.));
 N.Storm=H>=19||H<L::SmallHours?N.Wind:0.;
 if(D.bShower)
 {
  double Cloud=0;
  N.ShowerH=H-D.ShowerAt;
  ShowerCourse(N.ShowerH,D.bThunder,N.Rain,N.Wet,Cloud,N.bThunder);
 }
 if(D.bHaze)N.Haze=D.HazeK*L::Trap(H,4.,5.,8.,9.75);
 N.CloudBias=CloudBiasAt(D,ClockOfDay);
 const double From=FMath::DegreesToRadians(double(D.WindFrom));
 N.DirX=-FMath::Sin(From);N.DirZ=FMath::Cos(From);
 return N;
}
S47Weather::FLive& S47Weather::Live()
{
 static FLive Shared;
 return Shared;
}
S47Weather::FDay S47Weather::Forecast(int32 Seed,int32 IssuedOn,int32 ForDay)
{
 namespace L=S47WeatherLocal;
 FDay D=Day(Seed,ForDay);
 const int32 Ahead=FMath::Max(0,ForDay-IssuedOn),Salt=Seed*31+IssuedOn;
 // Off by a few degrees, the more the further ahead, as real forecasts are; the wind exact on the day.
 const double Spread=2.+Ahead*1.3;
 D.HighF=L::Clean(FMath::Clamp(D.HighF+FMath::RoundToInt((L::Draw(Salt,ForDay,20)-.5)*2.*Spread),60,95));
 D.LowF=L::Clean(FMath::Clamp(D.LowF+FMath::RoundToInt((L::Draw(Salt,ForDay,21)-.5)*2.*(1.+Ahead*.6)),36,48));
 if(Ahead>0)
 {
  D.WindMph=L::Clean(FMath::Clamp(D.WindMph+FMath::RoundToInt((L::Draw(Salt,ForDay,22)-.5)*2.*(1.+Ahead)),4,36));
  D.GustMph=L::Clean(FMath::Max(D.GustMph,D.WindMph+6));
 }
 // Two days out a stray shower is easily missed; three days out the sand too (the wind is still forecast). Never one that is not.
 if(Ahead>=2&&D.bShower&&L::Draw(Salt,ForDay,23)<.4){D.bShower=false;D.bThunder=false;}
 if(Ahead>=3&&D.bDust&&L::Draw(Salt,ForDay,24)<.25)D.bDust=false;
 return D;
}
FString S47Weather::Words(const FDay& D,bool bShort)
{
 namespace L=S47WeatherLocal;
 const FString AmSky=L::SkyWord(D.CloudAm),PmSky=L::SkyWord(D.CloudPm),DarkSky=L::SkyWord(D.CloudNight);
 if(bShort)
 {
  const FString What=D.bDust?FString(TEXT("Windy, blowing sand")):D.bShower?FString(TEXT("Stray shower")):D.Windy()?FString(TEXT("Windy")):D.bHaze?TEXT("Haze early, ")+PmSky:L::Capital(PmSky);
  return FString::Printf(TEXT("%s. %d/%d"),*What,D.HighF,D.LowF);
 }
 const FString Sky_=AmSky==PmSky?PmSky:FString::Printf(TEXT("%s in the morning, %s in the afternoon"),*AmSky,*PmSky);
 FString Said_=D.bHaze?TEXT("Haze early, then ")+Sky_:L::Capital(Sky_);
 Said_+=DarkSky==PmSky?TEXT("."):DarkSky==TEXT("clear")?TEXT(", clearing after sundown."):TEXT(", fewer clouds after sundown.");
 if(D.bDust)
 {
  Said_+=FString::Printf(TEXT(" %s wind %d to %d mph in the afternoon, gusts to %d. Blowing sand, with poor visibility at times."),*L::Capital(L::WayFrom(D)),L::Clean(D.WindMph-4),L::Clean(D.WindMph+4),D.GustMph);
  Said_+=D.NightStorm>L::HeldNight?TEXT(" Windy on through the night, easing toward morning."):TEXT(" Easing after sundown.");
 }
 else if(D.Windy())Said_+=FString::Printf(TEXT(" Windy in the afternoon, %s %d to %d mph, gusty."),*L::WayFrom(D),L::Clean(D.WindMph-4),L::Clean(D.WindMph+4));
 else
 {
  Said_+=D.WindMph<10?TEXT(" Light winds"):TEXT(" Light to moderate winds");
  Said_+=D.GustMph>0?TEXT(", gusty at times."):TEXT(".");
 }
 if(D.bShower)Said_+=D.bThunder?TEXT(" A stray shower late in the day, most of it drying up before it reaches the ground. Thunder possible."):TEXT(" A stray shower late in the day, most of it drying up before it reaches the ground.");
 if(D.bLightning)Said_+=TEXT(" Dry lightning far off after dark.");
 Said_+=FString::Printf(TEXT(" High %d, low %d."),D.HighF,D.LowF);
 return Said_;
}
FString S47Weather::LastWeek(int32 Seed,int32 FromDay,int32 ToDay)
{
 namespace L=S47WeatherLocal;
 int32 High=-100,Low=200,Showers=0,Sand=0;
 for(int32 D=FMath::Max(1,FromDay);D<ToDay;++D){const FDay W=Day(Seed,D);High=FMath::Max(High,W.HighF);Low=FMath::Min(Low,W.LowF);Showers+=W.bShower?1:0;Sand+=W.bDust?1:0;}
 if(High<-50)return TEXT("Caliche has no week of weather to report yet.");
 const FString Rain=Showers>0?FString(TEXT("a stray shower that hardly settled the road")):FString(TEXT("no rain"));
 const FString Blown=Sand>1?FString::Printf(TEXT(", blowing sand on %d afternoons"),L::Clean(Sand)):Sand==1?FString(TEXT(", blowing sand one afternoon")):FString();
 return FString::Printf(TEXT("Caliche last week: high %d, low %d, %s%s. Dry lightning far off most nights."),High,Low,*Rain,*Blown);
}
FString S47Weather::ClockWords(double SecondsAfterMidnight)
{
 const int32 Minutes=FMath::FloorToInt(FMath::Fmod(FMath::Fmod(SecondsAfterMidnight,86400.)+86400.,86400.)/60.);
 const int32 Hour24=Minutes/60,Minute=Minutes%60,Hour12=Hour24%12==0?12:Hour24%12;
 return FString::Printf(TEXT("%d:%02d %s"),Hour12,Minute,Hour24<12?TEXT("a.m."):TEXT("p.m."));
}
FString S47Weather::Almanac()
{
 return FString::Printf(TEXT("Sun rises %s, sets %s. Halley's comet is low in the south from dark until a little after three in the morning. It is fading now; binoculars help, away from town lights."),
  *ClockWords(SunriseClock),*ClockWords(SunsetClock));
}
FString S47Weather::Almanac(const FDateTime& Date)
{
 static const TCHAR* const Months[12]={TEXT("Jan."),TEXT("Feb."),TEXT("March"),TEXT("April"),TEXT("May"),TEXT("June"),TEXT("July"),TEXT("Aug."),TEXT("Sept."),TEXT("Oct."),TEXT("Nov."),TEXT("Dec.")};
 const auto Dated=[](const FDateTime& D){return FString::Printf(TEXT("%s %d"),Months[FMath::Clamp(D.GetMonth(),1,12)-1],D.GetDay());};
 double MoonUp=-1,MoonDown=-1;MoonTimes(Date,MoonUp,MoonDown);
 // "about": to the nearest five minutes (and so never the number the paper keeps out).
 const auto About=[](double& Clock){if(Clock>=0)Clock=FMath::Fmod(FMath::RoundToDouble(Clock/300.)*300.,86400.);};
 About(MoonUp);About(MoonDown);
 FString Moon=FString::Printf(TEXT(" Moon: %s"),*MoonPhase(Date));
 if(MoonUp>=0&&MoonDown>=0)Moon+=FString::Printf(TEXT(", rises about %s, sets about %s"),*ClockWords(MoonUp),*ClockWords(MoonDown));
 else if(MoonUp>=0)Moon+=FString::Printf(TEXT(", rises about %s"),*ClockWords(MoonUp));
 else if(MoonDown>=0)Moon+=FString::Printf(TEXT(", sets about %s"),*ClockWords(MoonDown));
 const FDateTime Full=NextPhase(Date,2),New=NextPhase(Date,0);
 Moon+=Full<New?FString::Printf(TEXT(". Full moon %s."),*Dated(Full)):FString::Printf(TEXT(". New moon %s."),*Dated(New));
 return Almanac()+Moon;
}
double S47Weather::MoonAge(const FDateTime& Date)
{
 namespace L=S47WeatherLocal;
 // Noon in the valley is seven in the evening at Greenwich.
 const FDateTime Noon(Date.GetYear(),Date.GetMonth(),Date.GetDay(),19,0,0);
 const double Age=FMath::Fmod(Noon.GetJulianDay()-L::EpochJd,L::Synodic);
 return Age<0?Age+L::Synodic:Age;
}
FString S47Weather::MoonPhase(const FDateTime& Date)
{
 const double A=MoonAge(Date);
 if(A<1.||A>=28.53)return TEXT("new moon");
 if(A<6.38)return TEXT("waxing crescent");
 if(A<8.38)return TEXT("first quarter");
 if(A<13.77)return TEXT("waxing gibbous");
 if(A<15.77)return TEXT("full moon");
 if(A<21.15)return TEXT("waning gibbous");
 if(A<23.15)return TEXT("last quarter");
 return TEXT("waning crescent");
}
S47Weather::FMoon S47Weather::Moon(const FDateTime& Date,double Clock)
{
 namespace L=S47WeatherLocal;
 FMoon Mo;
 // Mountain Standard Time is seven hours behind Greenwich; the clock may run past midnight into the next date.
 const double Jd=FDateTime(Date.GetYear(),Date.GetMonth(),Date.GetDay()).GetJulianDay()+(Clock+7*3600.)/86400.;
 Mo.Age=FMath::Fmod(Jd-L::EpochJd,L::Synodic);if(Mo.Age<0)Mo.Age+=L::Synodic;
 Mo.Lit=(1-FMath::Cos(2*UE_DOUBLE_PI*Mo.Age/L::Synodic))*.5;
 // The sun on the ecliptic, then the moon its elongation further east; both near enough to the ecliptic for this.
 const double N=Jd-2451545.;
 const double SunL=FMath::DegreesToRadians(FMath::Fmod(280.46+.9856474*N,360.)),SunG=FMath::DegreesToRadians(FMath::Fmod(357.528+.9856003*N,360.));
 const double SunLam=SunL+FMath::DegreesToRadians(1.915*FMath::Sin(SunG)+.02*FMath::Sin(2*SunG));
 const double Lam=SunLam+2*UE_DOUBLE_PI*Mo.Age/L::Synodic,Eps=FMath::DegreesToRadians(23.44);
 const double Ra=FMath::Atan2(FMath::Cos(Eps)*FMath::Sin(Lam),FMath::Cos(Lam)),Dec=FMath::Asin(FMath::Sin(Eps)*FMath::Sin(Lam));
 // Local sidereal time at the valley (106.6 W, 33.3 N).
 const double Gmst=FMath::Fmod(280.46061837+360.98564736629*N,360.);
 const double Lst=FMath::DegreesToRadians(Gmst-106.6),Lat=FMath::DegreesToRadians(33.3);
 const auto Place=[Lst,Lat](double InRa,double InDec,double& OutAz,double& OutEl)
 {
  const double Ha=Lst-InRa;
  OutEl=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FMath::Sin(Lat)*FMath::Sin(InDec)+FMath::Cos(Lat)*FMath::Cos(InDec)*FMath::Cos(Ha),-1.,1.)));
  OutAz=FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(-FMath::Sin(Ha),FMath::Tan(InDec)*FMath::Cos(Lat)-FMath::Sin(Lat)*FMath::Cos(Ha)))+360.,360.);
 };
 Place(Ra,Dec,Mo.Az,Mo.El);
 Place(FMath::Atan2(FMath::Cos(Eps)*FMath::Sin(SunLam),FMath::Cos(SunLam)),FMath::Asin(FMath::Sin(Eps)*FMath::Sin(SunLam)),Mo.SunAz,Mo.SunEl);
 return Mo;
}
float S47Weather::MoonLight(const FMoon& Mo)
{
 // Half lit is the night as tuned (1); new or below the horizon what the sky gives (.4); full and high 2.4. Low in the sky, less.
 const double ByPhase=Mo.Lit<=.5?.4+.6*(Mo.Lit/.5):1+1.4*((Mo.Lit-.5)/.5);
 const double Up=FMath::SmoothStep(-2.,12.,Mo.El);
 return float(FMath::Lerp(.4,ByPhase,Up));
}
void S47Weather::MoonTimes(const FDateTime& Date,double& OutRise,double& OutSet)
{
 OutRise=OutSet=-1;
 bool bWas=Moon(Date,0).El>0;
 for(int32 Minute=10;Minute<=24*60;Minute+=10)
 {
  const bool bUp=Moon(Date,Minute*60.).El>0;
  if(bUp&&!bWas&&OutRise<0)OutRise=Minute*60.;
  if(!bUp&&bWas&&OutSet<0)OutSet=Minute*60.;
  bWas=bUp;
 }
}
FDateTime S47Weather::NextPhase(const FDateTime& From,int32 Quarter)
{
 namespace L=S47WeatherLocal;
 const double Target=FMath::Clamp(Quarter,0,3)*L::Synodic/4.;
 const double Ahead=FMath::Fmod(Target-MoonAge(From)+2*L::Synodic,L::Synodic);
 return FDateTime(From.GetYear(),From.GetMonth(),From.GetDay())+FTimespan::FromDays(FMath::RoundToDouble(Ahead));
}
bool S47Weather::SelfTest(TArray<FString>& Lines)
{
 namespace L=S47WeatherLocal;
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s weather: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 bool bSame=true,bRange=true,bClean=true,bSky=true,bPrinted=true,bOnTheDay=true,bNeverInvented=true,bCalmStart=true,bNow=true,bSmooth=true,bWay=true;
 int32 Wrong=0,Days=0,Dusty=0,Showers=0,Hazy=0,Missed=0,ShowersAhead=0;
 for(const int32 TrySeed:{1,470047,-90210,123456789,777,31337})for(int32 Num=1;Num<=30;++Num)
 {
  const FDay A=Day(TrySeed,Num),B=Day(TrySeed,Num);++Days;
  bSame&=A.HighF==B.HighF&&A.LowF==B.LowF&&A.WindMph==B.WindMph&&A.GustMph==B.GustMph&&A.bDust==B.bDust&&A.DustK==B.DustK&&A.bShower==B.bShower&&A.ShowerAt==B.ShowerAt&&A.bHaze==B.bHaze&&A.NightStorm==B.NightStorm;
  bRange&=A.HighF>=64&&A.HighF<=92&&A.LowF>=39&&A.LowF<=44&&A.WindMph<=32&&A.GustMph>A.WindMph&&(!A.bDust||(A.DustK>=.45f&&A.DustK<=1.f&&A.DustFrom<A.DustTo));
  const auto Cover=[&A](double Clock){return float(FMath::Clamp(FS47Sky::CloudCoverAt(Clock)+CloudBiasAt(A,Clock),0.,.9));};
  bSky&=A.CloudAm==Cover(L::Morning)&&A.CloudPm==Cover(L::Afternoon)&&A.CloudNight==Cover(L::Night)&&A.bLightning;
  Dusty+=A.bDust?1:0;Showers+=A.bShower?1:0;Hazy+=A.bHaze?1:0;
  if(Num<=2)bCalmStart&=!A.bDust&&!A.bShower&&!A.Windy();
  // the paper on the day says what the day is; further out it may miss a shower, never invent one
  const FDay Today=Forecast(TrySeed,Num,Num);
  bOnTheDay&=Today.bDust==A.bDust&&Today.bShower==A.bShower&&Today.bHaze==A.bHaze&&Today.WindMph==A.WindMph;
  const FDay Said=Forecast(TrySeed,FMath::Max(1,Num-3),Num);
  bNeverInvented&=(!Said.bDust||A.bDust)&&(!Said.bShower||A.bShower);
  if(A.bShower&&Num>3){++ShowersAhead;Missed+=Said.bShower?0:1;}
  Wrong+=Said.HighF!=A.HighF?1:0;
  for(const FDay& Print:{A,Said})
  {
   const FString Long=Words(Print),Short=Words(Print,true);
   bPrinted&=Long.Contains(TEXT("blowing sand"))==Print.bDust&&Short.Contains(TEXT("blowing sand"))==Print.bDust;
   bPrinted&=Long.Contains(TEXT("stray shower"))==Print.bShower&&Long.Contains(TEXT("Haze early"))==Print.bHaze;
   for(const FString& Text:{Long,Short})
   {
    bClean&=!Text.Contains(TEXT("47"))&&!Text.IsEmpty();
    // the paper's own test keeps these out (S47Ledger): a sand day is blowing sand, a shower mostly virga
    bPrinted&=!Text.Contains(TEXT("dust"))&&!Text.Contains(TEXT("sprinkle"))&&!Text.Contains(TEXT("frost"))&&!Text.Contains(TEXT("rain"));
   }
  }
  // what the world shows over the day is the day's weather
  const FNow Noon=Now(TrySeed,Num,12*3600.),Four=Now(TrySeed,Num,16*3600.),Nine=Now(TrySeed,Num,9*3600.),Dawn=Now(TrySeed,Num,6.5*3600.),Late=Now(TrySeed,Num,11*3600.);
  bNow&=A.bDust?Four.Dust>.3:(Four.Dust==0&&Noon.Dust==0);
  bNow&=Nine.Dust==0&&Noon.Dust==0&&Dawn.Rain==0;
  if(A.bDust)bNow&=Four.Wind>Dawn.Wind&&Four.Wind>.3;
  if(!A.Windy())bNow&=Four.Wind<.05;
  if(A.bShower){const FNow In=Now(TrySeed,Num,(A.ShowerAt+.25)*3600.),Later=Now(TrySeed,Num,(A.ShowerAt+1.2)*3600.);bNow&=In.Rain>.9&&Later.Wet>.5&&Later.Rain==0&&In.CloudBias>.15;}
  else bNow&=Four.Rain==0&&Four.Wet==0;
  bNow&=(Dawn.Haze>.4)==A.bHaze&&Late.Haze==0;
  bNow&=Four.CloudBias==CloudBiasAt(A,16*3600.);
  // midnight is no seam: the small hours have what is left of the night before
  const FNow Before=Now(TrySeed,Num,86399.),After=Now(TrySeed,Num+1,1.);
  bSmooth&=FMath::Abs(Before.Mph-After.Mph)<1.;
  // it blows away from where the gusts come from
  const double From=FMath::DegreesToRadians(double(A.WindFrom));
  bWay&=FMath::IsNearlyEqual(Four.DirX*FMath::Sin(From)-Four.DirZ*FMath::Cos(From),-1.,1e-6)&&FMath::IsNearlyEqual(From,FS47Omen::WindFrom(TrySeed,Num),1e-4);
 }
 Check(bSame,TEXT("a seed and a day always give the same weather"));
 Check(bSky,TEXT("the paper's clouds are the ones FS47Sky draws with the day's weather on them, morning, afternoon and night, and dry lightning every night"));
 Check(bCalmStart,TEXT("the first two days of a game are calm: the station is learnt in plain weather"));
 Check(Dusty*100>=Days*8&&Dusty*100<=Days*34,FString::Printf(TEXT("blowing sand some afternoons, %d of %d days (spring here)"),Dusty,Days));
 Check(Showers*100>=Days*3&&Showers*100<=Days*16,FString::Printf(TEXT("a stray shower is rare, %d of %d days"),Showers,Days));
 Check(Hazy*100>=Days*8&&Hazy*100<=Days*40,FString::Printf(TEXT("haze some mornings, %d of %d"),Hazy,Days));
 Check(bRange,TEXT("highs 64 to 92 (a shower day cooler), the night down to the ambience's 41 F or so at dawn, wind up to about 30 mph"));
 Check(Wrong>20,TEXT("a forecast three days out is sometimes off in the warmth, like a real one"));
 Check(bOnTheDay,TEXT("the paper's TODAY is the day's own weather: sand, shower, haze and wind"));
 Check(bNeverInvented&&Missed>0&&Missed<ShowersAhead,TEXT("a forecast days out sometimes misses a shower and never invents sand or a shower"));
 Check(bPrinted,TEXT("the paper says blowing sand, a stray shower and haze early exactly when the day has them, and never dust, sprinkles, frost or rain"));
 Check(bClean,TEXT("nothing printed about the weather says the number the game keeps out"));
 Check(bNow,TEXT("what the world shows is the day's: sand in a dust day's afternoon only, rain at the shower's hour and slick dirt after, haze at dawn"));
 Check(bSmooth,TEXT("the wind runs on over midnight into the small hours without a jump"));
 {
  // M5 critic 3: a stormy night (FS47Rhythm's busier one) still blows in the night's own hours; a windy day without sand is still by night
  int32 Held=0,HeldWindy=0,Calm=0,CalmStill=0;bool bSeam=true;
  for(const int32 TrySeed:{1,470047,-90210,123456789,777,31337})for(int32 Num=1;Num<=30;++Num)
  {
   const FDay A=Day(TrySeed,Num);
   const FNow Small=Now(TrySeed,Num+1,2.5*3600.),Dawn6=Now(TrySeed,Num+1,6.*3600.),Dawn6Less=Now(TrySeed,Num+1,6.*3600.-1.);
   bSeam&=FMath::Abs(Dawn6.Mph-Dawn6Less.Mph)<1.&&FMath::Abs(Dawn6.Dust-Dawn6Less.Dust)<.01;
   if(A.NightStorm>L::HeldNight){++Held;HeldWindy+=Small.Wind>.1&&Small.Storm>.1?1:0;}
   else if(!A.bDust&&A.Windy()){++Calm;CalmStill+=A.NightStorm==0.f&&Small.Storm<.05?1:0;}
  }
  Check(Held>0&&HeldWindy==Held&&CalmStill==Calm&&bSeam,FString::Printf(TEXT("a stormy night blows on at half past two (%d of %d); a windy day without sand is still by night (%d of %d); no seam at six"),HeldWindy,Held,CalmStill,Calm));
 }
 Check(bWay,TEXT("the wind blows away from where FS47Omen's gusts come from"));
 {
  int32 Seed=-1,OnDay=-1;
  for(int32 TrySeed=1;TrySeed<40&&Seed<0;++TrySeed)for(int32 Num=3;Num<12;++Num)if(Day(TrySeed,Num).bShower){Seed=TrySeed;OnDay=Num;break;}
  Check(Seed>0&&LastWeek(Seed,OnDay-2,OnDay+2).Contains(TEXT("stray shower"))&&LastWeek(Seed,1,1).Contains(TEXT("no week")),TEXT("last week's weather tells a shower when there was one"));
 }
 {
  // The sky's sun: up just before sunrise's minute is gone and just after sunset's, down on the other sides.
  FS47Sky Sky;bool bSun=true;
  Sky.SetClock(SunriseClock-300);bSun&=Sky.SunDir.Y<0;Sky.SetClock(SunriseClock+300);bSun&=Sky.SunDir.Y>0;
  Sky.SetClock(SunsetClock-300);bSun&=Sky.SunDir.Y>0;Sky.SetClock(SunsetClock+300);bSun&=Sky.SunDir.Y<0;
  Check(bSun&&ClockWords(SunriseClock)==TEXT("5:30 a.m.")&&ClockWords(SunsetClock)==TEXT("6:27 p.m."),TEXT("sunrise and sunset in the almanac are the sky's own, 5:30 and 6:27"));
  bool bComet=true;
  Sky.SetCometClock(CometRises+60);bComet&=Sky.bComet;Sky.SetCometClock(CometSets-60);bComet&=Sky.bComet;
  Sky.SetCometClock(CometSets+60);bComet&=!Sky.bComet;Sky.SetCometClock(CometRises-60);bComet&=!Sky.bComet;
  Check(bComet&&Almanac().Contains(TEXT("comet"))&&!Almanac().Contains(TEXT("moon")),TEXT("the almanac's comet is up when the sky draws it, and no moon is printed while the sky has none"));
 }
 Check(MoonPhase(FDateTime(1986,4,24))==TEXT("full moon")&&MoonPhase(FDateTime(1986,5,8))==TEXT("new moon")&&MoonPhase(FDateTime(1986,5,1))==TEXT("last quarter"),TEXT("the moon of spring 1986 kept for MOON: full on April 24, last quarter May 1, new on May 8"));
 Check(NextPhase(FDateTime(1986,4,28),0)==FDateTime(1986,5,8)&&NextPhase(FDateTime(1986,5,9),2)==FDateTime(1986,5,23),TEXT("the next new and full moons after a date"));
 {
  // 23 May 1986 was a full moon: at midnight in the valley it stands in the south, fairly low (late May, far south on the ecliptic).
  const FMoon Full=Moon(FDateTime(1986,5,23),24*3600.);
  const FMoon New=Moon(FDateTime(1986,5,8),23*3600.);
  const FMoon First=Moon(FDateTime(1986,5,16),20*3600.);
  Check(Full.Lit>.97&&Full.El>15&&Full.Az>140&&Full.Az<220&&New.Lit<.03&&New.El<0,TEXT("the full moon of 23 May stands south at midnight; the new moon of 8 May is down at 23:00"));
  Check(First.Lit>.35&&First.Lit<.75&&First.Az>150&&First.Az<260&&First.El>20,TEXT("the first quarter stands south-west in the evening"));
  Check(FMath::IsNearlyEqual(MoonLight(FMoon{14.8,.5,180,50}),1.f,.02f)&&MoonLight(FMoon{14.8,1,180,60})>2.2f&&MoonLight(FMoon{0,0,0,-30})<.5f&&MoonLight(FMoon{14.8,1,90,-10})<.5f,TEXT("moonlight: a half moon is the night as tuned, a full one brighter, none down"));
  double MoonUp=0,MoonDown=0;MoonTimes(FDateTime(1986,5,23),MoonUp,MoonDown);
  Check(MoonUp>17*3600.&&MoonUp<21.5*3600.,TEXT("a full moon rises about sunset"));
  const FString Printed=Almanac(FDateTime(1986,5,16));
  Check(Printed.StartsWith(Almanac())&&Printed.Contains(TEXT("Moon: "))&&Printed.Contains(TEXT("Full moon May 23.")),TEXT("the paper's almanac prints the moon the sky draws, and the next full moon"));
 }
 return OK;
}
