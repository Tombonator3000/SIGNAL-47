#include "S47NightTiers.h"
#include "S47Freeform.h"
#include "S47NightRhythm.h"

namespace S47NightTiers
{
const TCHAR* Name(ES47Allowed Tier){return Tier==ES47Allowed::Full?TEXT("full"):Tier==ES47Allowed::Glimpse?TEXT("glimpse"):TEXT("hint");}
bool FromName(const FString& Text,ES47Allowed& Out)
{
 if(Text==TEXT("hint")){Out=ES47Allowed::Hint;return true;}
 if(Text==TEXT("glimpse")){Out=ES47Allowed::Glimpse;return true;}
 if(Text==TEXT("full")){Out=ES47Allowed::Full;return true;}
 return false;
}

bool SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s night tiers: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 const auto Is=[](double Hour,int32 Night,ES47Allowed T){return At(Hour,Night)==T;};
 // Week one (night 3), and the same on the second and third weeks.
 Check(Is(23.5,3,ES47Allowed::Hint)&&Is(19,3,ES47Allowed::Hint)&&Is(14,3,ES47Allowed::Hint),TEXT("23:30 in the first week holds hints only, as does the evening and the day"));
 Check(Is(.5,3,ES47Allowed::Glimpse)&&Is(0,3,ES47Allowed::Glimpse)&&Is(1.99,3,ES47Allowed::Glimpse),TEXT("00:30 is a glimpse, from midnight until two"));
 Check(Is(2+10/60.,3,ES47Allowed::Full)&&Is(2,3,ES47Allowed::Full)&&Is(4.5,20,ES47Allowed::Full),TEXT("02:10 is full, from two"));
 Check(Is(23+10/60.,21,ES47Allowed::Hint)&&Is(1+10/60.,21,ES47Allowed::Glimpse),TEXT("the last night of the third week has no creep yet"));
 // Week four.
 Check(Is(23+10/60.,22,ES47Allowed::Glimpse)&&Is(22.9,22,ES47Allowed::Hint)&&Is(1+10/60.,22,ES47Allowed::Full)&&Is(.9,28,ES47Allowed::Glimpse),TEXT("in the fourth week 23:10 is a glimpse and 01:10 full"));
 // The order: what holds more holds less too; an authored early start only when it is set.
 Check(Allows(ES47Allowed::Hint,2.5,3)&&Allows(ES47Allowed::Glimpse,2.5,3)&&Allows(ES47Allowed::Glimpse,.5,3)&&!Allows(ES47Allowed::Full,.5,3),TEXT("a full hour allows a glimpse and a hint, a glimpse hour not a full meeting"));
 Check(!Allows(ES47Allowed::Full,23.5,2)&&Allows(ES47Allowed::Full,23.5,2,true)&&!Allows(ES47Allowed::Glimpse,22,2,false),TEXT("an authored exception starts early only when it is set"));
 Check(FMath::IsNearlyEqual(FromClockHour(ES47Allowed::Full,3),2.)&&FMath::IsNearlyEqual(FromClockHour(ES47Allowed::Full,22),1.)&&FMath::IsNearlyEqual(FromClockHour(ES47Allowed::Glimpse,3),0.)&&FMath::IsNearlyEqual(FromClockHour(ES47Allowed::Glimpse,22),23.),TEXT("the edges as clock hours: 02:00 and 00:00, 01:00 and 23:00 in the fourth week"));
 {ES47Allowed T=ES47Allowed::Hint;Check(FromName(TEXT("glimpse"),T)&&T==ES47Allowed::Glimpse&&FromName(Name(ES47Allowed::Full),T)&&T==ES47Allowed::Full&&!FromName(TEXT("near"),T),TEXT("the names read back"));}
 // From the session's clock: the small hours belong to the evening before (FS47FreeformSession::NightOf).
 {
  const double Night4Mid=4*86400.;
  Check(AtClock(Night4Mid+.5*3600.,FS47FreeformSession::NightOf(Night4Mid+.5*3600.))==ES47Allowed::Glimpse&&AtClock(Night4Mid+2.2*3600.,FS47FreeformSession::NightOf(Night4Mid+2.2*3600.))==ES47Allowed::Full,TEXT("from the session's clock, on the night it belongs to"));
 }
 // Their hours (FS47NightRhythm::StartHour) never start before the tier's full hour, whatever the dice and however often taken.
 {
  bool bFloor=true;
  for(int32 Seed:{0,1,47,123456})for(int32 Night=1;Night<=40;++Night)for(int32 Taken:{0,4})
   bFloor&=FS47NightRhythm::StartHour(Seed,Night,Taken)>=FullClockHour(Night)-1e-9;
  Check(bFloor,TEXT("bodies start no sooner than the full hour: two, one in the fourth week"));
 }
 return OK;
}
}
