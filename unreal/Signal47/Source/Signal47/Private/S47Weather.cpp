#include "S47Weather.h"

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
 const TCHAR* const Points[]={TEXT("north"),TEXT("northeast"),TEXT("east"),TEXT("southeast"),TEXT("south"),TEXT("southwest"),TEXT("west"),TEXT("northwest")};
 const TCHAR* const PointsUp[]={TEXT("North"),TEXT("Northeast"),TEXT("East"),TEXT("Southeast"),TEXT("South"),TEXT("Southwest"),TEXT("West"),TEXT("Northwest")};
 int32 PointOf(int32 FromDegrees){return ((FMath::RoundToInt(double(FromDegrees)/45.)%8)+8)%8;}
}

S47Weather::FDay S47Weather::Day(int32 Seed,int32 ShiftDay)
{
 namespace L=S47WeatherLocal;
 FDay D;const int32 Num=FMath::Max(1,ShiftDay);
 // A slow warming through the spring (about 74 F at the end of April, 84 F at the end of May), spells of three days or so that
 // run warmer or colder, and the day's own bit.
 const double Trend=74.+.33*(Num-1),Spell=(L::Draw(Seed,Num/3,2)-.5)*9.,Own=(L::Draw(Seed,Num,3)-.5)*4.;
 // A front through from the north now and then: wind round to the north, colder, the only days with a chance of a shower.
 const bool bFront=L::Draw(Seed,Num,4)<.16;
 D.HighF=FMath::Clamp(FMath::RoundToInt(Trend+Spell+Own-(bFront?8.:0.)),60,93);
 D.LowF=FMath::Clamp(D.HighF-FMath::RoundToInt(31.+L::Draw(Seed,Num,5)*8.),30,56);
 const double Gust=L::Draw(Seed,Num,6);
 if(Gust<.1){D.WindMph=3+FMath::FloorToInt(L::Draw(Seed,Num,7)*4.);}                                 // calm
 else if(Gust<.42){D.WindMph=18+FMath::FloorToInt(L::Draw(Seed,Num,7)*13.);D.GustMph=D.WindMph+12+FMath::FloorToInt(L::Draw(Seed,Num,8)*8.);} // windy
 else D.WindMph=7+FMath::FloorToInt(L::Draw(Seed,Num,7)*10.);
 D.WindFrom=bFront?FMath::RoundToInt(330.+L::Draw(Seed,Num,9)*75.)%360:FMath::RoundToInt(200.+L::Draw(Seed,Num,9)*75.);
 D.bDust=D.WindMph>=20||(D.GustMph>=34&&!bFront);
 D.Cloud=float(.4+L::Draw(Seed,Num,10)*.3);
 D.bShower=bFront&&L::Draw(Seed,Num,11)<.45;
 D.bLightning=L::Draw(Seed,Num,12)<.85;
 D.HighF=L::Clean(D.HighF);D.LowF=L::Clean(D.LowF);D.WindMph=L::Clean(D.WindMph);D.GustMph=L::Clean(D.GustMph);
 return D;
}
S47Weather::FDay S47Weather::Forecast(int32 Seed,int32 IssuedOn,int32 ForDay)
{
 namespace L=S47WeatherLocal;
 FDay D=Day(Seed,ForDay);
 const int32 Ahead=FMath::Max(0,ForDay-IssuedOn),Salt=Seed*31+IssuedOn;
 const double Spread=2.+Ahead*1.3;
 D.HighF=L::Clean(FMath::Clamp(D.HighF+FMath::RoundToInt((L::Draw(Salt,ForDay,20)-.5)*2.*Spread),58,95));
 D.LowF=L::Clean(FMath::Clamp(D.LowF+FMath::RoundToInt((L::Draw(Salt,ForDay,21)-.5)*2.*Spread),28,58));
 if(D.LowF>D.HighF-18)D.LowF=L::Clean(D.HighF-18);
 D.WindMph=L::Clean(FMath::Clamp(D.WindMph+FMath::RoundToInt((L::Draw(Salt,ForDay,22)-.5)*2.*(2.+Ahead*2.)),3,40));
 if(D.GustMph>0)D.GustMph=L::Clean(FMath::Max(D.WindMph+8,D.GustMph+FMath::RoundToInt((L::Draw(Salt,ForDay,23)-.5)*8.)));
 // Now and then simply wrong, as real forecasts are.
 if(L::Draw(Salt,ForDay,24)<.08+.05*Ahead)D.bDust=!D.bDust;
 if(L::Draw(Salt,ForDay,25)<.04+.03*Ahead)D.bShower=!D.bShower;
 if(D.bDust&&D.WindMph<16)D.WindMph=L::Clean(16+FMath::FloorToInt(L::Draw(Salt,ForDay,26)*8.));
 return D;
}
FString S47Weather::Compass(int32 FromDegrees){return S47WeatherLocal::Points[S47WeatherLocal::PointOf(FromDegrees)];}
FString S47Weather::Words(const FDay& D,bool bShort)
{
 namespace L=S47WeatherLocal;
 const bool bWindy=D.WindMph>=18,bCalm=D.WindMph<7;
 FString Sky=D.Cloud<.47f?TEXT("Mostly sunny"):D.Cloud<.62f?TEXT("Partly cloudy"):TEXT("Clouds building in the afternoon");
 if(bShort)
 {
  FString Said_=D.bDust?TEXT("Windy, dust"):bWindy?TEXT("Windy"):D.WindMph>=13?TEXT("Breezy"):bCalm?TEXT("Light wind"):Sky;
  if(D.bShower)Said_+=TEXT(", sprinkles");
  return FString::Printf(TEXT("%s. %d/%d"),*Said_,D.HighF,D.LowF);
 }
 FString Said_=Sky;
 if(D.bDust)Said_+=TEXT(", windy, with blowing dust in the afternoon.");
 else if(bWindy)Said_+=TEXT(" and windy in the afternoon.");
 else if(D.WindMph>=13)Said_+=TEXT(" and breezy.");
 else if(bCalm)Said_+=TEXT(", light winds.");
 else Said_+=TEXT(".");
 if(!bCalm)
 {
  const int32 Lo=L::Clean(FMath::Max(5,(D.WindMph/5)*5)),Hi=L::Clean(Lo+10);
  Said_+=FString::Printf(TEXT(" %s %d to %d"),L::PointsUp[L::PointOf(D.WindFrom)],Lo,Hi);
  Said_+=D.GustMph>0?FString::Printf(TEXT(", gusts to %d."),L::Clean((D.GustMph/5)*5)):FString(TEXT("."));
 }
 if(D.bShower)Said_+=TEXT(" A few sprinkles possible, most of it drying before it reaches the ground.");
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
FDateTime S47Weather::NextPhase(const FDateTime& From,int32 Quarter)
{
 namespace L=S47WeatherLocal;
 const double Target=FMath::Clamp(Quarter,0,3)*L::Synodic/4.;
 const double Ahead=FMath::Fmod(Target-MoonAge(From)+2*L::Synodic,L::Synodic);
 return FDateTime(From.GetYear(),From.GetMonth(),From.GetDay())+FTimespan::FromDays(FMath::RoundToDouble(Ahead));
}
bool S47Weather::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s weather: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 bool bSame=true,bRange=true,bClean=true;int32 Dusty=0,Showers=0,Calm=0,Wrong=0;
 for(const int32 TrySeed:{1,470047,-90210,123456789})for(int32 Num=1;Num<=30;++Num)
 {
  const FDay A=Day(TrySeed,Num),B=Day(TrySeed,Num);
  bSame&=A.HighF==B.HighF&&A.LowF==B.LowF&&A.WindMph==B.WindMph&&A.bDust==B.bDust&&A.bShower==B.bShower;
  bRange&=A.HighF>=60&&A.HighF<=93&&A.LowF>=30&&A.LowF<=A.HighF-25&&A.Cloud>=.4f&&A.Cloud<=.7f;
  Dusty+=A.bDust?1:0;Showers+=A.bShower?1:0;Calm+=A.WindMph<7?1:0;
  const FDay Said=Forecast(TrySeed,FMath::Max(1,Num-3),Num);
  Wrong+=Said.HighF!=A.HighF||Said.bDust!=A.bDust?1:0;
  for(const FString& Text:{Words(A),Words(A,true),Words(Said),Words(Said,true)})bClean&=!Text.Contains(TEXT("47"))&&!Text.IsEmpty();
 }
 Check(bSame,TEXT("a seed and a day always give the same weather"));
 Check(bRange,TEXT("spring in the valley: highs 60 to 93, cold nights, the sky's broken layer"));
 Check(Dusty>=12&&Dusty<=60,TEXT("windy, dusty afternoons are common in the spring"));
 Check(Showers<=20,TEXT("rain is rare: a few sprinkles in a month"));
 Check(Calm>0,TEXT("and now and then a still day"));
 Check(Wrong>20,TEXT("a forecast three days out is sometimes wrong, like a real one"));
 Check(bClean,TEXT("nothing printed about the weather says the number the game keeps out"));
 Check(ClockWords(SunriseClock)==TEXT("5:30 a.m.")&&ClockWords(SunsetClock)==TEXT("6:27 p.m."),TEXT("sunrise and sunset are the sky's own, 5:30 and 6:27"));
 Check(MoonPhase(FDateTime(1986,4,24))==TEXT("full moon")&&MoonPhase(FDateTime(1986,5,8))==TEXT("new moon")&&MoonPhase(FDateTime(1986,5,1))==TEXT("last quarter"),TEXT("the moon of spring 1986: full on April 24, last quarter May 1, new on May 8"));
 Check(NextPhase(FDateTime(1986,4,28),0)==FDateTime(1986,5,8)&&NextPhase(FDateTime(1986,5,9),2)==FDateTime(1986,5,23),TEXT("the next new and full moons after a date"));
 return OK;
}
