#include "S47Thoughts.h"
#include "HAL/PlatformTime.h"
#include "S47Main.h"
#include "S47UI.h"

// D46-5-20261110
namespace
{
 using FPool=TArray<FString>;
 const TMap<FString,FPool>& Pools()
 {
  static const TMap<FString,FPool> P=[]
  {
   TMap<FString,FPool> R;
   // Heard or felt as it starts.
   R.Add(TEXT("car_dies"),{TEXT("No. Not now."),TEXT("Come on. Come on."),TEXT("Everything just quit."),TEXT("That's not the battery.")});
   R.Add(TEXT("dogs_hush"),{TEXT("Why did the dogs stop?"),TEXT("Too quiet out there."),TEXT("Not a bark. Nothing."),TEXT("Even the dogs went quiet.")});
   R.Add(TEXT("phone_empty"),{TEXT("Hello? Hello?"),TEXT("Who was that?"),TEXT("Somebody was on the line."),TEXT("Nobody there. Again.")});
   R.Add(TEXT("town_blackout"),{TEXT("Whole town just went dark."),TEXT("Power's out. Great."),TEXT("Every window, gone at once."),TEXT("Now what?")});
   R.Add(TEXT("array_burst"),{TEXT("What was that on the trace?"),TEXT("That wasn't on the chart."),TEXT("The dishes picked up something."),TEXT("Is it the gear, or is it me?")});
   R.Add(TEXT("am_caller"),{TEXT("Who calls a radio at this hour?"),TEXT("What did he see out there?"),TEXT("Hope he's just had a few.")});
   R.Add(TEXT("cb_stranded"),{TEXT("Somebody's out there."),TEXT("Hope he's all right."),TEXT("Who's out on the road this late?")});
   R.Add(TEXT("cb_echo"),{TEXT("That's not right."),TEXT("That sounded familiar."),TEXT("Who is this?")});
   R.Add(TEXT("room6_tv"),{TEXT("Who left that on?"),TEXT("Just static. It's just static."),TEXT("Turn it off. Please.")});
   R.Add(TEXT("cinema_film"),{TEXT("Nobody's running that projector."),TEXT("Who's showing a picture at this hour?"),TEXT("The doors were locked.")});
   R.Add(TEXT("house_lit"),{TEXT("Nobody lives there."),TEXT("Whose light is that?"),TEXT("That house was dark."),TEXT("Was that on before?")});
   R.Add(TEXT("animals_flee"),{TEXT("What spooked them?"),TEXT("They felt it first."),TEXT("Whatever it was, they ran.")});
   R.Add(TEXT("car_light"),{TEXT("Headlights, but no engine."),TEXT("Is somebody following me?"),TEXT("Who's out here?")});
   R.Add(TEXT("dust_devil"),{TEXT("Wind. Just wind."),TEXT("Where did that come from?"),TEXT("Funny, there's no breeze.")});
   // Seen, far or near; the hours decide what comes (S47NightTiers), these only say what a tired man says.
   R.Add(TEXT("landing"),{TEXT("Those lights. What are those?"),TEXT("I swear I saw something."),TEXT("That's no plane."),TEXT("Did anybody else see that?")});
   R.Add(TEXT("horizon_figure"),{TEXT("I swear I saw something."),TEXT("Probably a post. Probably."),TEXT("Too far off to be anybody.")});
   R.Add(TEXT("road_glimpse"),{TEXT("What was that in the road?"),TEXT("A deer. It was a deer."),TEXT("I swear I saw something.")});
   R.Add(TEXT("hitchhiker"),{TEXT("Who walks out here?"),TEXT("I swear I saw somebody."),TEXT("Don't stop. Don't stop.")});
   R.Add(TEXT("figure_near"),{TEXT("Is somebody out there?"),TEXT("No. Not again."),TEXT("I swear I saw something."),TEXT("Keep walking.")});
   R.Add(TEXT("big_night"),{TEXT("Okay. This is different."),TEXT("No. Not again."),TEXT("Everybody's seeing it tonight."),TEXT("Stay calm. Stay calm.")});
   // The places' own: a face at a window or a door, something over the ceiling, any noise.
   R.Add(TEXT("peek"),{TEXT("What was that noise?"),TEXT("Something at the window?"),TEXT("I'm just tired. That's all."),TEXT("Don't look. Don't look.")});
   R.Add(TEXT("roof"),{TEXT("Is somebody on the roof?"),TEXT("What was that noise?"),TEXT("Birds. Has to be birds.")});
   R.Add(TEXT("noise"),{TEXT("What was that noise?"),TEXT("Did you hear that?"),TEXT("I swear I heard something.")});
   return R;
  }();
  return P;
 }
 int32 Last_(const FString& K,int32 Set=-2)
 {
  static TMap<FString,int32> L;
  if(Set>=-1){L.Add(K,Set);return Set;}
  const int32* F=L.Find(K);return F?*F:-1;
 }
 double LastAt=-1e9;
}

namespace S47Thoughts
{
 const TArray<FString>& Pool(const FString& Key)
 {
  static const TArray<FString> None;
  const FPool* P=Pools().Find(Key);return P?*P:None;
 }
 const TArray<FString>& Keys()
 {
  static TArray<FString> K;
  if(K.Num()==0)Pools().GetKeys(K);
  return K;
 }
 int32 PickIndex(int32 Count,int32 Last,double Roll)
 {
  if(Count<=0)return -1;
  if(Count==1)return 0;
  // Roll over the lines but the last one shown.
  const int32 N=Count-1;
  int32 I=FMath::Clamp(int32(FMath::Clamp(Roll,0.,.9999)*N),0,N-1);
  if(Last>=0&&Last<Count&&I>=Last)++I;
  return I;
 }
 bool May(double NowReal,double LastReal){return NowReal-LastReal>=Cooldown;}
 bool Think(US47Main* M,const FString& Key)
 {
  if(!IsValid(M)||!M->UI||M->ModalOpen())return false;
  const FPool& P=Pool(Key);if(P.Num()==0)return false;
  const double Now=FPlatformTime::Seconds();
  if(!May(Now,LastAt))return false;
  const int32 I=PickIndex(P.Num(),Last_(Key),FMath::FRand());
  if(I<0)return false;
  LastAt=Now;Last_(Key,I);
  M->UI->Thought(P[I],4.f);
  UE_LOG(LogTemp,Display,TEXT("S47: thought (%s): %s"),*Key,*P[I]);
  return true;
 }
 bool SelfTest(TArray<FString>& Lines)
 {
  bool bOk=true;
  auto Check=[&](bool C,const FString& What){Lines.Add(FString(C?TEXT("PASS "):TEXT("FAIL "))+What);bOk&=C;};
  static const TCHAR* const Banned[]={TEXT("47"),TEXT("1947"),TEXT("cable"),TEXT("tomas"),TEXT("taken"),TEXT("silhouette"),TEXT("8.15"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("martian"),TEXT("third")};
  FString Bad;int32 Lines_=0;bool bSize=true,bShort=true;
  for(const FString& K:Keys())
  {
   const FPool& P=Pool(K);Lines_+=P.Num();
   if(P.Num()<3||P.Num()>6)bSize=false;
   TSet<FString> Seen;
   for(const FString& L:P)
   {
    if(L.Len()>MaxLen||L.Len()<8||L.TrimStartAndEnd()!=L||S47Thought::Is(L)){bShort=false;Bad=L;}
    if(Seen.Contains(L)){bShort=false;Bad=L;}
    Seen.Add(L);
    const FString Lo=L.ToLower();
    for(const TCHAR* B:Banned)if(Lo.Contains(B))Bad=L;
    for(const TCHAR C:L)if(C<32||C>126)Bad=L;
    // a standalone capital C is a spoiler too
    for(int32 X=0;X<L.Len();++X)if(L[X]=='C'&&(X==0||!FChar::IsAlpha(L[X-1]))&&(X+1>=L.Len()||!FChar::IsAlpha(L[X+1])))Bad=L;
   }
  }
  Check(Keys().Num()>=20&&bSize,FString::Printf(TEXT("D46-5: %d pools of 3 to 6 lines (%d lines)"),Keys().Num(),Lines_));
  Check(bShort,TEXT("D46-5: every line is short, plain, once in its pool: ")+Bad);
  Check(Bad.IsEmpty(),TEXT("D46-5: no spoiler word, no odd character: ")+Bad);
  // the cooldown and the pick
  Check(!May(100.,60.)&&May(150.,60.)&&May(1e6,-1e9),TEXT("D46-5: one thought in 90 real seconds"));
  bool bPick=true;
  for(int32 N=2;N<=6;++N)for(int32 L=-1;L<N;++L)for(int32 R=0;R<=10;++R){const int32 I=PickIndex(N,L,R/10.);if(I<0||I>=N||I==L)bPick=false;}
  Check(bPick&&PickIndex(0,-1,.5)==-1&&PickIndex(1,0,.5)==0,TEXT("D46-5: a pick is in range and never the last line"));
  Check(Pool(TEXT("no_such")).Num()==0&&Pool(TEXT("peek")).Num()>0&&!Think(nullptr,TEXT("peek")),TEXT("D46-5: unknown key or no game: nothing"));
  return bOk;
 }
}
