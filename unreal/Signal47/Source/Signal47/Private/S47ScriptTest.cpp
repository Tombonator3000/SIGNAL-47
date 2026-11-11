#include "S47Script.h"
#include "S47ScriptPlayer.h"
#include "S47Sky.h"
#include "S47Weather.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// DIRECTOR-TOOL-20261109: the scripted events' self-test, called from the HUD's (-S47SelfTest): the JSON both ways, the trigger,
// the night's tiers (S47NightTiers: a figure waits for two, a glimpse for midnight, sooner in the fourth week, early only when set),
// the lock against a big event, and the lint. No world: FS47ScriptNow is filled by hand (and the sky's words by FS47ScriptPlayer::SkyNow,
// which is pure).

namespace S47ScriptTestLocal
{
 FS47Script Sample()
 {
  FS47Script S;S.Id=TEXT("test_all");S.Name=TEXT("Everything once");S.bEnabled=true;S.Repeat=ES47ScriptRepeat::Nightly;
  S.Trigger.DayFrom=3;S.Trigger.DayTo=20;S.Trigger.HourFrom=23.5;S.Trigger.HourTo=4;S.Trigger.Place=TEXT("farm");S.Trigger.Radius=60;
  FS47ScriptCond C1;C1.If=TEXT("finding");C1.Arg=TEXT("cattle_ring*");
  FS47ScriptCond C2;C2.If=TEXT("case_open");C2.Num=3;
  FS47ScriptCond C3;C3.If=TEXT("weather");C3.Arg=TEXT("clear");
  FS47ScriptCond C4;C4.If=TEXT("moon_max");C4.Num=.4;
  FS47ScriptCond C5;C5.If=TEXT("outdoors");
  FS47ScriptCond C6;C6.If=TEXT("near");C6.Arg=TEXT("church");C6.Num=900;
  S.Trigger.Conds={C1,C2,C3,C4,C5,C6};
  const auto Add=[&S](ES47ScriptAct Do,double At)->FS47ScriptAction&{FS47ScriptAction A;A.Do=Do;A.At=At;return S.Actions.Add_GetRef(A);};
  FS47ScriptPoint P;P.X=120.5;P.Z=-40;P.Up=2;P.bSet=true;
  {FS47ScriptAction& A=Add(ES47ScriptAct::Sky,0);A.Kind=TEXT("triangle");A.Who=TEXT("slow");}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Sound,3);A.Text=TEXT("thudSoft");A.Who=TEXT("placed");A.Point=P;A.Gain=.8;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Line,5);A.Who=TEXT("dale");A.Text=TEXT("You seeing that? Over.");}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Dish,6);A.Az=200;A.El=12;A.Slew=6;A.Secs=30;A.Index=4;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Fault,8);A.Kind=TEXT("omen");A.Gain=.5;A.Secs=12;A.Point=P;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Wait,9);A.Secs=10;A.bFlag=true;A.Point=P;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Toast,10);A.Text=TEXT("The crickets have stopped.");A.Secs=4;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Note,11);A.Title=TEXT("On the door");A.Text=TEXT("Back at six. D.");}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Evidence,12);A.Kind=TEXT("script_test_all");A.Text=TEXT("Lights over the farm, three, slow.");}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Look,13);A.Point=P;A.Secs=1.5;}
  {FS47ScriptAction& A=Add(ES47ScriptAct::Figure,14);A.Kind=TEXT("tall");A.Speed=.9;A.Secs=5;A.bFlag=true;A.Path={P,P};A.Path[1].X+=20;A.Path[0].Up=0;A.Path[1].Up=0;}
  return S;
 }
 FS47ScriptNow NightAt(double Hour,int32 Day=5)
 {
  FS47ScriptNow N;N.Day=Day;N.Night=Day;N.Clock=(Day-1)*86400.+Hour*3600.;
  if(Hour<8)N.Clock+=86400.; // the small hours belong to the next calendar day, still this night
  N.bHasRoad=true;N.Road=FVector2D(10,10);
  N.Weather={TEXT("clear"),TEXT("calm")};N.MoonLit=.2;
  N.HasFinding=[](const FString& Id){return Id==TEXT("cattle_ring*")||Id.StartsWith(TEXT("cattle_ring"));};
  N.CaseState=[](int32 L){return L==3?1:0;};
  N.PlaceAt=[](const FString& Id,FVector2D& At,double& R){if(Id==TEXT("farm")){At=FVector2D(0,0);R=12;return true;}if(Id==TEXT("church")){At=FVector2D(500,500);R=12;return true;}return false;};
  return N;
 }
}

bool S47Script::SelfTest(TArray<FString>& Lines)
{
 using namespace S47ScriptTestLocal;
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString(bOk?TEXT("PASS "):TEXT("FAIL "))+TEXT("scripts: ")+What);};

 // ---- the JSON both ways ----
 {
  const FS47Script S=Sample();
  const FString A=Write(S);
  FS47Script R;FString Why;
  const bool bRead=Read(A,R,Why);
  Check(bRead&&Write(R)==A,TEXT("written, read and written again: the same text"));
  Check(bRead&&R.Actions.Num()==S.Actions.Num()&&R.Trigger.Conds.Num()==6&&R.Trigger.Place==TEXT("farm")&&R.Repeat==ES47ScriptRepeat::Nightly,TEXT("every action and condition, the place and the repeat survive"));
  const FS47ScriptAction* Fig=R.Actions.FindByPredicate([](const FS47ScriptAction& X){return X.Do==ES47ScriptAct::Figure;});
  Check(Fig&&Fig->Kind==TEXT("tall")&&Fig->Path.Num()==2&&FMath::IsNearlyEqual(Fig->Path[1].X,140.5)&&Fig->bFlag,TEXT("a figure keeps its body, its path and gone-when-seen"));
  const FS47ScriptAction* Wt=R.Actions.FindByPredicate([](const FS47ScriptAction& X){return X.Do==ES47ScriptAct::Wait;});
  const FS47ScriptAction* Di=R.Actions.FindByPredicate([](const FS47ScriptAction& X){return X.Do==ES47ScriptAct::Dish;});
  Check(Di&&Di->Index==4&&FMath::IsNearlyEqual(Di->Az,200.)&&FMath::IsNearlyEqual(Di->Secs,30.),TEXT("an antenna turned on its own keeps its number, bearing and hold"));
  Check(Wt&&Wt->bFlag&&Wt->Point.bSet&&FMath::IsNearlyEqual(Wt->Point.Up,2.),TEXT("a wait until looked at keeps its point and its height"));
  FS47Script Bad;
  Check(!Read(TEXT("{\"version\":2,\"id\":\"x\"}"),Bad,Why)&&!Read(TEXT("not json"),Bad,Why),TEXT("another version or broken text is refused"));
  Check(!Read(TEXT("{\"version\":1,\"id\":\"x\",\"actions\":[{\"at\":0,\"do\":\"explode\"}]}"),Bad,Why)&&Why.Contains(TEXT("explode")),TEXT("an unknown action is refused by name"));
  Check(FMath::IsNearlyEqual(ParseHour(TEXT("23:30")),23.5)&&FMath::IsNearlyEqual(ParseHour(TEXT("2.25")),2.25)&&ParseHour(TEXT("x"))<0&&HourText(1.5)==TEXT("01:30"),TEXT("hours read as 23:30 or 2.25, and written back"));
  Check(CleanId(TEXT("Lights over the Playa!"))==TEXT("lights_over_the_playa"),TEXT("an id from a name"));
  // The example that comes with the game: there, and switched off.
  const FString Example=FPaths::ProjectContentDir()/TEXT("Director/scripted/example_lights_over_playa.json");
  FString Text;
  if(FFileHelper::LoadFileToString(Text,*Example))
  {
   FS47Script E;
   const bool bOk=Read(Text,E,Why);
   Check(bOk&&!E.bEnabled&&Lint(E).Num()==0,TEXT("the example reads, keeps to the limits, and is off"));
  }
  else Lines.Add(TEXT("NOTE scripts: the example file is not in this build (Content/Director/scripted)"));
 }

 // ---- the trigger ----
 {
  FS47Script S=Sample();
  // (a light in the sky is a glimpse at least, CRIT6-20261109: the trigger is tried on the plain hints)
  S.Actions.RemoveAll([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure||A.Do==ES47ScriptAct::Sky;});
  FS47ScriptMemory Mem;FString Why;
  Check(Ready(S,NightAt(23.75),Mem,Why),TEXT("in its hours, days, place and conditions it starts")+(Why.IsEmpty()?FString():TEXT(" (")+Why+TEXT(")")));
  Check(Ready(S,NightAt(3.5),Mem,Why),TEXT("its hours run past midnight"));
  Check(!Ready(S,NightAt(22),Mem,Why)&&Why.Contains(TEXT("hours")),TEXT("before its hours it waits"));
  Check(!Ready(S,NightAt(23.75,2),Mem,Why)&&!Ready(S,NightAt(23.75,21),Mem,Why),TEXT("outside its days it waits"));
  FS47ScriptNow Far=NightAt(23.75);Far.Road=FVector2D(200,0);
  Check(!Ready(S,Far,Mem,Why)&&Why.Contains(TEXT("place")),TEXT("away from its place it waits"));
  FS47ScriptNow In=NightAt(23.75);In.bIndoors=true;
  Check(!Ready(S,In,Mem,Why)&&Why.Contains(TEXT("outdoors")),TEXT("a condition that does not hold (outdoors) keeps it back"));
  FS47ScriptNow Moon=NightAt(23.75);Moon.MoonLit=.9;
  Check(!Ready(S,Moon,Mem,Why),TEXT("too much moon keeps it back"));
  FS47ScriptNow Closed=NightAt(23.75);Closed.CaseState=[](int32){return 2;};
  Check(!Ready(S,Closed,Mem,Why),TEXT("a closed case is not an open one"));
  FS47ScriptNow Cloud=NightAt(23.75);Cloud.Weather={TEXT("cloudy")};
  Check(!Ready(S,Cloud,Mem,Why),TEXT("the weather has to be the one asked for"));
  FS47ScriptNow NoFind=NightAt(23.75);NoFind.HasFinding=[](const FString&){return false;};
  Check(!Ready(S,NoFind,Mem,Why),TEXT("a finding it needs, missing, keeps it back"));
  // How often.
  Mem.Played(S.Id,5,NightAt(23.75).Clock);
  Check(!Ready(S,NightAt(1),Mem,Why)&&Why.Contains(TEXT("tonight")),TEXT("nightly: once a night"));
  Check(Ready(S,NightAt(23.75,6),Mem,Why),TEXT("nightly: again the next night"));
  S.Repeat=ES47ScriptRepeat::Once;
  Check(!Ready(S,NightAt(23.75,6),Mem,Why),TEXT("once: never again"));
  S.Repeat=ES47ScriptRepeat::Always;S.CooldownMin=60;
  Check(!Ready(S,NightAt(0.25),Mem,Why)&&Ready(S,NightAt(1),Mem,Why),TEXT("always: again after its cooldown"));
  S.bEnabled=false;
  Check(!Ready(S,NightAt(1),Mem,Why)&&Ready(S,NightAt(1),Mem,Why,true),TEXT("switched off it does not start, but plays as a preview"));
  S.bEnabled=true;
  FS47ScriptNow Big=NightAt(1);Big.bBigEvent=true;Big.BigWhy=TEXT("the Director's event close_pass");
  Check(!Ready(S,Big,Mem,Why)&&Why.Contains(TEXT("close_pass")),TEXT("never while another big event holds the night"));
  // The memory is saved and read back.
  FS47ScriptMemory Back;Back.Load(S47J::Read(S47J::Write(Mem.Save())));
  Check(Back.Count.FindRef(S.Id)==1&&Back.LastNight.FindRef(S.Id)==5,TEXT("what has been played is saved and read back"));
 }

 // ---- nothing alien before two ----
 {
  FS47Script S=Sample();
  S.Trigger.HourFrom=23;S.Trigger.HourTo=5;
  FS47ScriptMemory Mem;FString Why;
  Check(!Ready(S,NightAt(23.5),Mem,Why)&&Why.Contains(TEXT("02:00")),TEXT("a figure at 23:30 waits for 02:00"));
  Check(!Ready(S,NightAt(1.9),Mem,Why),TEXT("a figure at 01:54 still waits"));
  Check(Ready(S,NightAt(2.1),Mem,Why),TEXT("a figure at 02:06 plays"));
  S.Trigger.HourFrom=-1;S.Trigger.HourTo=-1;
  Check(!Ready(S,NightAt(14),Mem,Why)&&!Ready(S,NightAt(6.5),Mem,Why),TEXT("a figure with no hours of its own waits for 02:00 too, by day as well"));
  Check(Ready(S,NightAt(23.5),Mem,Why,true)&&Why.Contains(TEXT("02:00")),TEXT("a preview plays now and says that in play it would wait"));
  FS47Script NoFig=Sample();NoFig.Actions.RemoveAll([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure||A.Do==ES47ScriptAct::Sky;});
  Check(Ready(NoFig,NightAt(23.75),Mem,Why),TEXT("a sound and a voice are not held for 02:00"));
  // CRIT6-20261109: a formation in the sky with no figure is a glimpse: not before midnight.
  FS47Script Lights=Sample();Lights.Actions.RemoveAll([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure;});
  Check(Lights.Need()==ES47Allowed::Glimpse&&!Ready(Lights,NightAt(23.75),Mem,Why)&&Why.Contains(TEXT("00:00"))&&Ready(Lights,NightAt(.5),Mem,Why),TEXT("CRIT6: lights in the sky without a figure are a glimpse: they wait at 23:45 and play at 00:30"));
  TArray<FString> Warn;
  FS47Script Never=Sample();Never.Trigger.HourFrom=19;Never.Trigger.HourTo=23;
  Check(Lint(Never,&Warn).Num()==0&&Warn.ContainsByPredicate([](const FString& W){return W.Contains(TEXT("never play"));}),TEXT("a figure whose hours never reach 02:00 gets a warning"));
 }

 // ---- the night's tiers (TIERS-20261109) ----
 {
  FS47ScriptMemory Mem;FString Why;
  FS47Script S=Sample();S.Trigger.DayTo=0;S.Trigger.HourFrom=22;S.Trigger.HourTo=5;
  Check(!Ready(S,NightAt(23+10/60.,22),Mem,Why)&&Ready(S,NightAt(1+10/60.,22),Mem,Why),TEXT("in the fourth week a full figure waits at 23:10 and plays at 01:10"));
  Check(!Ready(S,NightAt(1+10/60.,8),Mem,Why)&&Why.Contains(TEXT("02:00")),TEXT("in the second week it still waits for 02:00 at 01:10"));
  FS47Script G=S;G.Tier=ES47Allowed::Glimpse;
  Check(Lint(G).Num()==0&&!Ready(G,NightAt(23.5),Mem,Why)&&Ready(G,NightAt(.5),Mem,Why)&&!Ready(G,NightAt(22.9,22),Mem,Why)&&Ready(G,NightAt(23+10/60.,22),Mem,Why),TEXT("a glimpse: not at 23:30, at 00:30; in the fourth week from 23:00"));
  FS47Script Stays=G;for(FS47ScriptAction& A:Stays.Actions)if(A.Do==ES47ScriptAct::Figure)A.bFlag=false;
  FS47Script OnlyHint=S;OnlyHint.Tier=ES47Allowed::Hint;
  Check(Lint(Stays).Num()>0&&Lint(OnlyHint).Num()>0,TEXT("a glimpse whose figure stays is refused, and a figure is never only a hint"));
  FS47Script Early=S;Early.Trigger.HourFrom=-1;Early.Trigger.HourTo=-1;Early.bEarly=true;
  FS47Script NotEarly=Early;NotEarly.bEarly=false;
  Check(!Ready(NotEarly,NightAt(23.5),Mem,Why)&&Ready(Early,NightAt(23.5),Mem,Why)&&!Ready(Early,NightAt(14),Mem,Why),TEXT("an authored early start plays at 23:30 only when it is set, and never by day"));
  FS47Script Back;FS47Script EG=Early;EG.Tier=ES47Allowed::Glimpse;
  Check(Read(Write(EG),Back,Why)&&Back.bEarly&&Back.Tier==ES47Allowed::Glimpse&&!Write(S).Contains(TEXT("\"tier\""))&&!Write(S).Contains(TEXT("\"early\"")),TEXT("tier and early are written when set, and read back"));
  Check(!Read(Write(S).Replace(TEXT("\"id\""),TEXT("\"tier\": \"near\", \"id\"")),Back,Why),TEXT("an unknown tier is turned away"));
  // CRIT6-20261109: a tier the file sets holds for a script without a figure too, is written back, and the lint says what it does.
  FS47Script Plain=S;Plain.Actions.RemoveAll([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure||A.Do==ES47ScriptAct::Sky;});
  FS47Script Set=Plain;Set.Tier=ES47Allowed::Full;Set.bTierSet=true;
  TArray<FString> Warn;
  Check(Plain.Need()==ES47Allowed::Hint&&Ready(Plain,NightAt(23.5),Mem,Why)&&Set.Need()==ES47Allowed::Full&&!Ready(Set,NightAt(23.5),Mem,Why)&&Why.Contains(TEXT("02:00"))&&Ready(Set,NightAt(2.5),Mem,Why),TEXT("CRIT6: \"tier\": \"full\" with no figure waits for 02:00; with no tier set it is a hint at any hour"));
  Check(Read(Write(Set),Back,Why)&&Back.bTierSet&&Back.Tier==ES47Allowed::Full&&Write(Set).Contains(TEXT("\"tier\""))&&Read(Write(Plain),Back,Why)&&!Back.bTierSet,TEXT("CRIT6: a tier set on purpose is written and read back as set, even full"));
  Check(Lint(Set,&Warn).Num()==0&&Warn.ContainsByPredicate([](const FString& W){return W.Contains(TEXT("no figure"));}),TEXT("CRIT6: the lint warns that a tier with no figure holds everything the script does"));
  FS47Script Sky=Sample();Sky.Actions.RemoveAll([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure;});
  Check(Lint(Sky,&Warn).Num()==0&&Warn.ContainsByPredicate([](const FString& W){return W.Contains(TEXT("light in the sky"));}),TEXT("CRIT6: the lint warns that a light in the sky waits for the glimpse's hour"));
 }

 // ---- the weather and the moon in the small hours (CRIT6-20261109) ----
 {
  // 01:00 on the fourth calendar day is still the third shift day: a script reads the night's sky, as the world draws it.
  const double Clock=3*86400.+3600.;
  const FDateTime Date4(1986,4,24);
  int32 Wrong=0,OldWrong=0;
  for(int32 Seed=1;Seed<=300;++Seed)
  {
   FS47ScriptNow N;FS47ScriptPlayer::SkyNow(Seed,Clock,3,Date4,N);
   const bool bCloudy=FMath::Clamp(FS47Sky::CloudCoverAt(3600.)+S47Weather::Now(Seed,4,3600.).CloudBias,0.,.9)>=.5;
   Wrong+=N.Weather.Contains(TEXT("cloudy"))!=bCloudy;
   OldWrong+=(S47Weather::Day(Seed,3).CloudAm>=.5f)!=bCloudy;
  }
  Check(Wrong==0&&FS47ScriptPlayer::CalendarDayOf(Clock)==4&&FS47ScriptPlayer::CalendarDayOf(3*86400.-1)==3,FString::Printf(TEXT("CRIT6: at 01:00 the cover is the night's on the calendar day, not the shift day's morning (the old reading was wrong on %d of 300 games)"),OldWrong));
  FS47ScriptNow N;FS47ScriptPlayer::SkyNow(47,Clock,3,Date4,N);
  const S47Weather::FMoon Mo=S47Weather::Moon(Date4,3600.);
  Check(FMath::IsNearlyEqual(N.MoonLit,Mo.Lit)&&N.bMoonUp==(Mo.El>0)&&N.Weather.Num()>=2,TEXT("CRIT6: the moon a script reads at 01:00 is the calendar date's, as the sky draws it"));
 }

 // ---- the lint ----
 {
  const TCHAR* const Refused[]={
   TEXT("It lasted 47 seconds."),TEXT("forty-seven seconds of nothing"),TEXT("The clock said 05:29:47."),
   TEXT("the photograph from 1947"),TEXT("back in '47"),
   TEXT("Meet me at C."),TEXT("Point C, east of the station"),
   TEXT("Somebody cut the cable."),TEXT("the cable was severed"),
   TEXT("Tomás is still out there"),TEXT("Tomas Vega"),
   TEXT("a third silhouette on the print"),TEXT("the third figure"),
   TEXT("the bolt at 8.15"),TEXT("eight fifteen miles out"),
   TEXT("This is how they took him."),TEXT("Reyes was taken by the light"),TEXT("they beamed him up")};
  bool bAllRefused=true;FString Missed;
  for(const TCHAR* T:Refused)if(LintText(T).IsEmpty()){bAllRefused=false;Missed+=FString(T)+TEXT(" | ");}
  Check(bAllRefused,TEXT("every spoiler is refused")+(Missed.IsEmpty()?FString():TEXT(": missed ")+Missed));
  const TCHAR* const Kept[]={
   TEXT("1420.405 MHz, 27 antennas, az 026."),TEXT("SIGNAL / 47"),TEXT("CB 19, anybody out there?"),
   TEXT("Check the S-03 cable junction."),TEXT("Somebody took the truck into town."),TEXT("It was 23:47 when the lights came."),
   TEXT("Reyes was on nights before you."),TEXT("C'mon, it's cold."),TEXT("How long it took you to get here."),TEXT("The cattle stand in a ring."),TEXT("It's 41 degrees out.")};
  bool bAllKept=true;FString Wrong;
  for(const TCHAR* T:Kept)if(!LintText(T).IsEmpty()){bAllKept=false;Wrong+=FString(T)+TEXT(" (")+LintText(T)+TEXT(") | ");}
  Check(bAllKept,TEXT("ordinary words and the canon numbers pass")+(Wrong.IsEmpty()?FString():TEXT(": refused ")+Wrong));
  FS47Script S=Sample();
  S.Actions[2].Text=TEXT("Tomás says hello.");
  FS47ScriptMemory Mem;FString Why;
  Check(Lint(S).Num()==1&&!Ready(S,NightAt(23.75),Mem,Why)&&!Ready(S,NightAt(23.75),Mem,Why,true),TEXT("a script that says a spoiler is neither played nor previewed"));
  FS47Script Empty;Empty.Id=TEXT("Bad Id");
  Check(Lint(Empty).Num()>=2,TEXT("a bad id and no actions are refused"));
 }

 // ---- the shot (SHOT-BEAT-01) ----
 {
  FS47Script One;One.Id=TEXT("test_shot");One.Name=TEXT("A shot at the array");
  FS47ScriptPoint Eye;Eye.X=36;Eye.Z=-318;Eye.Up=1.7;Eye.bSet=true;
  FS47ScriptPoint Eye2=Eye;Eye2.X=28;Eye2.Z=-332;Eye2.Up=2.5;
  FS47ScriptPoint Mark;Mark.X=18;Mark.Z=-370;Mark.Up=9;Mark.bSet=true;
  {FS47ScriptAction A;A.Do=ES47ScriptAct::Shot;A.At=10;A.Path={Eye};A.Point=Mark;A.Fov=55;A.Secs=6;A.In=.6;A.Out=.8;One.Actions.Add(A);}
  FS47Script Two=One;Two.Actions[0].Path.Add(Eye2);Two.Actions[0].Fov2=40;
  FString Why;FS47Script R1,R2;
  const FString T1=Write(One),T2=Write(Two);
  const bool bR1=Read(T1,R1,Why),bR2=Read(T2,R2,Why);
  Check(bR1&&Write(R1)==T1&&R1.Actions.Num()==1&&R1.Actions[0].Do==ES47ScriptAct::Shot&&R1.Actions[0].Path.Num()==1&&FMath::IsNearlyEqual(R1.Actions[0].Path[0].Up,1.7)
   &&R1.Actions[0].Point.bSet&&FMath::IsNearlyEqual(R1.Actions[0].Point.Up,9.)&&FMath::IsNearlyEqual(R1.Actions[0].Fov,55.)&&R1.Actions[0].Fov2==0&&FMath::IsNearlyEqual(R1.Actions[0].Secs,6.)
   &&T1.Contains(TEXT("\"eye\""))&&!T1.Contains(TEXT("eye2"))&&!T1.Contains(TEXT("fov2")),TEXT("SHOT: a shot with one eye is written, read and written again the same, without eye2 or fov2"));
  Check(bR2&&Write(R2)==T2&&R2.Actions[0].Path.Num()==2&&FMath::IsNearlyEqual(R2.Actions[0].Path[1].X,28.)&&FMath::IsNearlyEqual(R2.Actions[0].Path[1].Up,2.5)&&FMath::IsNearlyEqual(R2.Actions[0].Fov2,40.)
   &&FMath::IsNearlyEqual(R2.Actions[0].In,.6)&&FMath::IsNearlyEqual(R2.Actions[0].Out,.8),TEXT("SHOT: a shot with a second eye keeps both eyes, both lenses and its ease in and out"));
  FS47Script Dflt;
  Check(Read(TEXT("{\"version\":1,\"id\":\"x\",\"actions\":[{\"at\":0,\"do\":\"shot\",\"eye2\":{\"x\":1,\"z\":2},\"point\":{\"x\":9,\"z\":9}}]}"),Dflt,Why)&&Dflt.Actions.Num()==1&&Dflt.Actions[0].Path.IsEmpty()
   &&FMath::IsNearlyEqual(Dflt.Actions[0].Fov,60.)&&FMath::IsNearlyEqual(Dflt.Actions[0].Secs,6.)&&FMath::IsNearlyEqual(Dflt.Actions[0].In,.6)&&Lint(Dflt).Num()>0,TEXT("SHOT: the defaults, and no second eye without a first (then refused: no eye)"));
  Check(Lint(One).Num()==0&&Lint(Two).Num()==0,TEXT("SHOT: the lint takes a good shot with one eye and with two"));
  Check(FMath::IsNearlyEqual(One.Length(),10+.6+6+.8),FString::Printf(TEXT("SHOT: the script lasts until the shot has eased back (%.1f s)"),One.Length()));
  // Each rule refuses on its own.
  struct FRule{const TCHAR* What;TFunction<void(FS47ScriptAction&)> Spoil;};
  const TArray<FRule> Rules={
   {TEXT("no eye"),[](FS47ScriptAction& X){X.Path.Reset();}},
   {TEXT("three eyes"),[](FS47ScriptAction& X){const FS47ScriptPoint First=X.Path[0];X.Path.Add(First);}},
   {TEXT("no point"),[](FS47ScriptAction& X){X.Point=FS47ScriptPoint();}},
   {TEXT("held 0.5 s"),[](FS47ScriptAction& X){X.Secs=.5;}},
   {TEXT("held 31 s"),[](FS47ScriptAction& X){X.Secs=31;}},
   {TEXT("fov 15"),[](FS47ScriptAction& X){X.Fov=15;}},
   {TEXT("fov 95"),[](FS47ScriptAction& X){X.Fov=95;}},
   {TEXT("fov2 10"),[](FS47ScriptAction& X){X.Fov2=10;}},
   {TEXT("in 0.2 s"),[](FS47ScriptAction& X){X.In=.2;}},
   {TEXT("out 4 s"),[](FS47ScriptAction& X){X.Out=4;}},
   {TEXT("an eye on the point"),[](FS47ScriptAction& X){X.Path[1]=X.Point;X.Path[1].X+=.2;}},
   {TEXT("eyes 250 m apart"),[](FS47ScriptAction& X){X.Path[1].X=X.Path[0].X+250;}},
   {TEXT("an eye under the ground"),[](FS47ScriptAction& X){X.Path[0].Up=-1;}},
   {TEXT("a point 301 m up"),[](FS47ScriptAction& X){X.Point.Up=301;}}};
  FString Let;
  for(const FRule& Rule:Rules){FS47Script B=Two;Rule.Spoil(B.Actions[0]);if(Lint(B).Num()==0)Let+=FString(Rule.What)+TEXT(" | ");}
  Check(Let.IsEmpty(),TEXT("SHOT: the lint refuses each rule on its own")+(Let.IsEmpty()?FString():TEXT(": let through ")+Let));
  // An older file, without a shot: read and written as before, nothing of the shot's in it.
  const FString Old=TEXT("{\"version\":1,\"id\":\"old\",\"name\":\"old\",\"enabled\":false,\"repeat\":\"once\",\"trigger\":{},\"actions\":[{\"at\":0,\"do\":\"look\",\"secs\":1.5,\"point\":{\"x\":4,\"z\":5}},{\"at\":2,\"do\":\"toast\",\"text\":\"Wind.\",\"secs\":4}]}");
  FS47Script O1,O2;
  const bool bOld=Read(Old,O1,Why)&&Read(Write(O1),O2,Why);
  const FString W1=Write(O1);
  Check(bOld&&Write(O2)==W1&&!W1.Contains(TEXT("\"fov\""))&&!W1.Contains(TEXT("\"eye\""))&&!W1.Contains(TEXT("\"in\""))&&!W1.Contains(TEXT("\"out\""))&&!Write(Sample()).Contains(TEXT("\"fov\""))
   &&O1.Actions.Num()==2&&FMath::IsNearlyEqual(O1.Actions[0].Secs,1.5)&&FMath::IsNearlyEqual(O1.Length(),6.),TEXT("SHOT: an older file without a shot reads and writes as before, with nothing of the shot's in it"));
  // The example that comes with the game: there, and switched off.
  const FString Example=FPaths::ProjectContentDir()/TEXT("Director/scripted/example_shot_array.json");
  FString Text;
  if(FFileHelper::LoadFileToString(Text,*Example))
  {
   FS47Script E;
   const bool bOk=Read(Text,E,Why);
   Check(bOk&&!E.bEnabled&&Lint(E).Num()==0&&E.Actions.ContainsByPredicate([](const FS47ScriptAction& X){return X.Do==ES47ScriptAct::Shot&&X.Path.Num()==2;}),TEXT("SHOT: the shot example reads, keeps to the limits, has a moving shot, and is off"));
  }
  else Lines.Add(TEXT("NOTE scripts: the shot example is not in this build (Content/Director/scripted)"));
 }
 return bAll;
}
