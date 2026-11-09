#include "S47Weather.h"
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
}

S47Weather::FDay S47Weather::Day(int32 Seed,int32 ShiftDay)
{
 namespace L=S47WeatherLocal;
 FDay D;const int32 Num=FMath::Max(1,ShiftDay);
 // The sky's own cover, the same every day.
 D.CloudAm=float(FS47Sky::CloudCoverAt(L::Morning));D.CloudPm=float(FS47Sky::CloudCoverAt(L::Afternoon));D.CloudNight=float(FS47Sky::CloudCoverAt(L::Night));
 // The warmth, which nobody sees: a slow climb through the spring (about 76 F at the end of April, 85 F at the end of May), spells
 // of three days or so that run warmer or colder, and the day's own bit. The night comes down to the ambience's dawn.
 const double Trend=76.+.3*(Num-1),Spell=(L::Draw(Seed,Num/3,2)-.5)*8.,Own=(L::Draw(Seed,Num,3)-.5)*4.;
 D.HighF=FMath::Clamp(FMath::RoundToInt(Trend+Spell+Own),68,92);
 D.LowF=L::DawnF-1+FMath::FloorToInt(L::Draw(Seed,Num,5)*3.);
 // The light wind the brush sways in (S47Wind) and the ambience's gusts now and then.
 D.WindMph=6+FMath::FloorToInt(L::Draw(Seed,Num,7)*6.);
 D.GustMph=D.WindMph+9+FMath::FloorToInt(L::Draw(Seed,Num,8)*6.);
 D.bDust=false;D.bShower=false;D.bLightning=true;
 D.HighF=L::Clean(D.HighF);D.LowF=L::Clean(D.LowF);D.WindMph=L::Clean(D.WindMph);D.GustMph=L::Clean(D.GustMph);
 return D;
}
S47Weather::FDay S47Weather::Forecast(int32 Seed,int32 IssuedOn,int32 ForDay)
{
 namespace L=S47WeatherLocal;
 FDay D=Day(Seed,ForDay);
 const int32 Ahead=FMath::Max(0,ForDay-IssuedOn),Salt=Seed*31+IssuedOn;
 // Off by a few degrees, the more the further ahead, as real forecasts are; never a dust storm or a shower the sky will not have.
 const double Spread=2.+Ahead*1.3;
 D.HighF=L::Clean(FMath::Clamp(D.HighF+FMath::RoundToInt((L::Draw(Salt,ForDay,20)-.5)*2.*Spread),64,95));
 D.LowF=L::Clean(FMath::Clamp(D.LowF+FMath::RoundToInt((L::Draw(Salt,ForDay,21)-.5)*2.*(1.+Ahead*.6)),36,48));
 D.WindMph=L::Clean(FMath::Clamp(D.WindMph+FMath::RoundToInt((L::Draw(Salt,ForDay,22)-.5)*2.*(1.+Ahead)),4,15));
 return D;
}
FString S47Weather::Words(const FDay& D,bool bShort)
{
 namespace L=S47WeatherLocal;
 const FString AmSky=L::SkyWord(D.CloudAm),PmSky=L::SkyWord(D.CloudPm),DarkSky=L::SkyWord(D.CloudNight);
 if(bShort)return FString::Printf(TEXT("%s. %d/%d"),*L::Capital(PmSky),D.HighF,D.LowF);
 FString Said_=AmSky==PmSky?L::Capital(PmSky):FString::Printf(TEXT("%s in the morning, %s in the afternoon"),*L::Capital(AmSky),*PmSky);
 Said_+=DarkSky==PmSky?TEXT("."):DarkSky==TEXT("clear")?TEXT(", clearing after sundown."):TEXT(", fewer clouds after sundown.");
 Said_+=D.WindMph<10?TEXT(" Light winds"):TEXT(" Light to moderate winds");
 Said_+=D.GustMph>0?TEXT(", gusty at times."):TEXT(".");
 if(D.bLightning)Said_+=TEXT(" Dry lightning far off after dark.");
 Said_+=FString::Printf(TEXT(" High %d, low %d."),D.HighF,D.LowF);
 return Said_;
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
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s weather: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 bool bSame=true,bRange=true,bClean=true,bSky=true,bNothingElse=true;int32 Wrong=0;
 for(const int32 TrySeed:{1,470047,-90210,123456789})for(int32 Num=1;Num<=30;++Num)
 {
  const FDay A=Day(TrySeed,Num),B=Day(TrySeed,Num);
  bSame&=A.HighF==B.HighF&&A.LowF==B.LowF&&A.WindMph==B.WindMph&&A.GustMph==B.GustMph;
  bRange&=A.HighF>=68&&A.HighF<=92&&A.LowF>=39&&A.LowF<=44&&A.WindMph<=15;
  bSky&=A.CloudAm==float(FS47Sky::CloudCoverAt(L::Morning))&&A.CloudPm==float(FS47Sky::CloudCoverAt(L::Afternoon))&&A.CloudNight==float(FS47Sky::CloudCoverAt(L::Night))&&A.bLightning;
  const FDay Said=Forecast(TrySeed,FMath::Max(1,Num-3),Num);
  bNothingElse&=!A.bDust&&!A.bShower&&!Said.bDust&&!Said.bShower;
  Wrong+=Said.HighF!=A.HighF?1:0;
  for(const FString& Text:{Words(A),Words(A,true),Words(Said),Words(Said,true)})
  {
   bClean&=!Text.Contains(TEXT("47"))&&!Text.IsEmpty();
   bNothingElse&=!Text.Contains(TEXT("dust"))&&!Text.Contains(TEXT("shower"))&&!Text.Contains(TEXT("sprinkle"))&&!Text.Contains(TEXT("rain"));
  }
 }
 Check(bSame,TEXT("a seed and a day always give the same weather"));
 Check(bSky,TEXT("the paper's clouds are the ones FS47Sky draws, morning, afternoon and night, and dry lightning every night as the sky has it"));
 Check(bNothingElse,TEXT("no dust, shower or rain in the weather or the forecast: the sky draws none"));
 Check(bRange,TEXT("highs 68 to 92, and the night down to the ambience's 41 F or so at dawn"));
 Check(Wrong>20,TEXT("a forecast three days out is sometimes off in the warmth, like a real one"));
 Check(bClean,TEXT("nothing printed about the weather says the number the game keeps out"));
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
