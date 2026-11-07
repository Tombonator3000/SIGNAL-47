#pragma once
#include "CoreMinimal.h"

// Freeform only (Tom, 7 November): the evening is work, the small hours are theirs. When they come is its own thing each
// night: drawn from the game's seed, so a Continue never rerolls it, and growing with the nights played. Pure: no world,
// no global dice. Numbers chosen for feel, for Tom to judge at the wheel.
struct SIGNAL47_API FS47NightRhythm
{
 static constexpr double EarliestHour=1,LatestHour=3,EndHour=6;
 static uint32 Mix(uint32 V){V^=V>>16;V*=0x7feb352du;V^=V>>15;V*=0x846ca68bu;return V^(V>>16);}
 // 0 to 1, always the same for this game, night and purpose.
 static float Roll(int32 Seed,int32 Night,uint32 Salt){return float(Mix(uint32(Seed)^Mix(uint32(Night)+0x4e495445u)^Mix(Salt))%100000u)/100000.f;}
 // 0 on the first night, about .33 on the third, .75 on the eighth. It only ever grows.
 static double Growth(int32 Night){return 1.-FMath::Exp(-FMath::Max(0,Night-1)/5.);}
 // Fully calm: no bodies, no lights, no second calls that lead anywhere. Never two running, none before the third night.
 static bool Quiet(int32 Seed,int32 Night){return RawQuiet(Seed,Night)&&!RawQuiet(Seed,Night-1);}
 // Hours past midnight when bodies can start: late at first, earlier as the nights go on, and earlier for someone already taken.
 static double StartHour(int32 Seed,int32 Night,int32 TimesTaken=0)
 {
  const double Base=FMath::Lerp(3.,1.25,Growth(Night))+(Roll(Seed,Night,0x53544152u)-.5)*1.;
  return FMath::Clamp(Base-FMath::Min(1.,.25*FMath::Max(0,TimesTaken)),EarliestHour,LatestHour);
 }
 static bool BodyHours(int32 Seed,int32 Night,double HourOfDay,int32 TimesTaken=0)
 {return !Quiet(Seed,Night)&&HourOfDay>=StartHour(Seed,Night,TimesTaken)&&HourOfDay<EndHour;}
 // Whatever comes to marks left behind, and to someone on foot in the open: these grow too, slowly.
 static float VisitChance(int32 Night){return float(FMath::Lerp(.45,.8,Growth(Night)));}
 static float FieldChance(int32 Night){return float(FMath::Lerp(.15,.55,Growth(Night)));}
 static double Threat(int32 Night){return FMath::Lerp(.75,1.25,Growth(Night));}
 // A day the array's ordinary service need comes up: no longer every third. About one day in three, one in seven with the
 // relay reserve; never two running, never the first day. The hour is its own draw, from ten until four.
 static bool FaultDay(int32 Seed,int32 Day,bool bReserve){return RawFault(Seed,Day,bReserve)&&!RawFault(Seed,Day-1,bReserve);}
 static double FaultHour(int32 Seed,int32 Day){return 10.+6.*Roll(Seed,Day,0x48524c59u);}
 static bool SelfTest(TArray<FString>& Lines);
private:
 static bool RawQuiet(int32 Seed,int32 Night){return Night>=3&&Roll(Seed,Night,0x51554945u)<float(FMath::Lerp(.3,.15,Growth(Night)));}
 static bool RawFault(int32 Seed,int32 Day,bool bReserve){return Day>=2&&Roll(Seed,Day,0x4641554cu)<(bReserve?.18f:.42f);}
};
