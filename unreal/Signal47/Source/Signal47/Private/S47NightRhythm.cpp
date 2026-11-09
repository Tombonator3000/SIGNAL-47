#include "S47NightRhythm.h"

bool FS47NightRhythm::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s night rhythm: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 double Early=0,Late=0;int32 Quiets=0,Runs=0,Nights=0,Faults=0,Reserve=0,FaultRuns=0,Days=0,Taken=0;bool bBounds=true,bStable=true,bHoursRight=true;
 for(int32 Seed:{0,1,47,-1,MAX_int32,123456,987654,31337})
 {
  for(int32 Night=1;Night<=120;++Night)
  {
   const double S=StartHour(Seed,Night);
   bBounds&=S>=EarliestHour&&S>=S47NightTiers::FullClockHour(Night)&&S<=LatestHour;bStable&=S==StartHour(Seed,Night);
   if(Night<=3)Late+=S;if(Night>=22&&Night<=24)Early+=S;
   Taken+=StartHour(Seed,Night,3)<=S?1:0;
   ++Nights;const bool Q=Quiet(Seed,Night);Quiets+=Q?1:0;Runs+=Q&&Quiet(Seed,Night-1)?1:0;
   bHoursRight&=!BodyHours(Seed,Night,.5)&&!BodyHours(Seed,Night,6.)&&!BodyHours(Seed,Night,20.)&&!BodyHours(Seed,Night,S-.01)&&BodyHours(Seed,Night,S+.01)==!Q;
   ++Days;const bool F=FaultDay(Seed,Night,false);Faults+=F?1:0;Reserve+=FaultDay(Seed,Night,true)?1:0;FaultRuns+=F&&FaultDay(Seed,Night-1,false)?1:0;
   bBounds&=FaultHour(Seed,Night)>=10&&FaultHour(Seed,Night)<=16;
  }
  bBounds&=!Quiet(Seed,1)&&!Quiet(Seed,2)&&!FaultDay(Seed,1,false)&&!FaultDay(Seed,1,true);
 }
 Check(bBounds,TEXT("bodies start between the full tier's hour (02:00, 01:00 in the fourth week) and 03:00, faults from ten until four, the first nights are never calm and the first day has no fault"));
 Check(bStable,TEXT("the same game and night always gives the same hour"));
 Check(Late/24>2.3&&Early/24<1.75,TEXT("the start comes earlier as the nights go on (the first nights against the fourth week's)"));
 Check(Taken==Nights,TEXT("those who have been taken are never come for later"));
 Check(bHoursRight,TEXT("body hours follow the night's own start, and end at six"));
 Check(Runs==0&&Quiets>Nights/20&&Quiets<Nights/3,TEXT("some nights are calm, never two running"));
 Check(FaultRuns==0&&Faults>Days/6&&Faults<Days/2&&Reserve<Faults,TEXT("array faults fall on uneven days, and the relay reserve makes them rarer"));
 Check(VisitChance(1)<VisitChance(9)&&FieldChance(1)<FieldChance(9)&&Threat(1)<Threat(9),TEXT("every chance grows with the nights"));
 return OK;
}
