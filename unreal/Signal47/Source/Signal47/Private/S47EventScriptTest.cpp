#include "S47EventScript.h"
#include "S47Freeform.h"
#include "S47NightRhythm.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// D46-20261110 (D46-11): the event scripts' self-test, called from the HUD's (-S47SelfTest). No world: every script there is (built
// in, and every file in the folders it is read from) parses, names only beats and conditions that exist and keeps to the rules; the
// three wired events keep the beats they had in code; the gates and the words, by hand.

namespace S47EventScriptTestLocal
{
 FS47ScriptNow At(int32 Day,double Hour)
 {
  FS47ScriptNow N;N.Day=Day;N.Night=Day;N.Clock=(Day-1)*86400.+Hour*3600.;
  if(Hour<8)N.Clock+=86400.; // the small hours belong to the next calendar day, still this night
  N.bIndoors=true;N.Weather={TEXT("clear"),TEXT("calm")};
  return N;
 }
 int32 CountOf(const FS47EventScript& S,ES47Beat Do){int32 N=0;for(const FS47Beat& B:S.Beats)N+=B.Do==Do;return N;}
 const FS47Beat* First(const FS47EventScript& S,ES47Beat Do){return S.Beats.FindByPredicate([Do](const FS47Beat& B){return B.Do==Do;});}
}

bool S47EventScript::SelfTest(TArray<FString>& Lines)
{
 using namespace S47EventScriptTestLocal;
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString(bOk?TEXT("PASS "):TEXT("FAIL "))+TEXT("event scripts: ")+What);};

 // ---- every built-in one: it parses, it is clean, it is wired and asks only for what its event gives ----
 TMap<FString,FS47EventScript> Built;
 for(const auto& P:BuiltIn())
 {
  FS47EventScript S;FString Why;TArray<FString> Warn;
  const bool bRead=Read(P.Value,S,Why);
  const TArray<FString> Errors=bRead?Lint(S,&Warn):TArray<FString>();
  Check(bRead&&S.Id==P.Key&&Errors.IsEmpty()&&Warn.IsEmpty()&&Wired().Contains(S.Id),FString::Printf(TEXT("built in %s parses, is clean and is played by its event%s"),*P.Key,
   !bRead?*(TEXT(": ")+Why):Errors.Num()?*(TEXT(": ")+Errors[0]):Warn.Num()?*(TEXT(": ")+Warn[0]):TEXT("")));
  if(bRead)Built.Add(S.Id,S);
 }
 for(const FString& Id:Wired())Check(Built.Contains(Id),FString::Printf(TEXT("%s, wired, has its beats built in"),*Id));

 // ---- the three that were code: the same beats ----
 if(const FS47EventScript* S=Built.Find(TEXT("array_burst")))
 {
  const FS47Beat* Tone=First(*S,ES47Beat::Tone);const FS47Beat* Pr=First(*S,ES47Beat::Printer);const FS47Beat* Ev=First(*S,ES47Beat::Evidence);const FS47Beat* To=First(*S,ES47Beat::Toast);const FS47Beat* Mp=First(*S,ES47Beat::Map);
  Check(Tone&&Tone->Repeat==6&&FMath::IsNearlyEqual(Tone->Hz,1400.)&&FMath::IsNearlyEqual(Tone->Every,.12)&&FMath::IsNearlyEqual(Tone->Gain,.06)&&Tone->Kind==TEXT("square"),TEXT("array_burst: six square beeps at 1400 Hz, .12 s apart"));
  Check(Pr&&FMath::IsNearlyEqual(Pr->Gain,.3)&&FMath::IsNearlyEqual(Pr->Delay,1.)&&FMath::IsNearlyEqual(Pr->Stop,3.),TEXT("array_burst: the printer from one second, stopped at three"));
  Check(Ev&&Ev->At==0&&Ev->Key==TEXT("array_burst_d{burst_night}")&&Ev->Text.Contains(TEXT("{az} degrees from SARO, give or take {spread}")),TEXT("array_burst: the finding at once, under the burst's night, with the bearing and the spread"));
  Check(To&&FMath::IsNearlyEqual(To->At,.8)&&FMath::IsNearlyEqual(To->Secs,5.)&&To->Text.Contains(TEXT("{map_key}")),TEXT("array_burst: the line on the screen after .8 s, for 5 s"));
  Check(Mp&&Mp->At==0&&Mp->Name==TEXT("{target}")&&S->Beats[0].Do==ES47Beat::Map,TEXT("array_burst: the place is heard of on the map first"));
 }
 if(const FS47EventScript* S=Built.Find(TEXT("town_blackout")))
 {
  const FS47Beat* L=First(*S,ES47Beat::Light);
  Check(L&&L->Kind==TEXT("town")&&CountOf(*S,ES47Beat::Tone)==2&&CountOf(*S,ES47Beat::Evidence)==1&&First(*S,ES47Beat::Evidence)->Key==TEXT("blackout_d{day}"),TEXT("town_blackout: the lamps, the two hums and the finding"));
  const FS47Beat* Hum=First(*S,ES47Beat::Tone);
  Check(Hum&&FMath::IsNearlyEqual(Hum->Hz,60.)&&FMath::IsNearlyEqual(Hum->Secs,40.)&&FMath::IsNearlyEqual(Hum->Delay,1.)&&Hum->Kind==TEXT("sine"),TEXT("town_blackout: the 60 Hz hum, forty seconds from one"));
 }
 if(const FS47EventScript* S=Built.Find(TEXT("hood_warm")))
 {
  const int32 Ticks=CountOf(*S,ES47Beat::Sound);const FS47Beat* T=First(*S,ES47Beat::Sound);
  Check(Ticks==2&&T&&T->Name==TEXT("hap:tick")&&T->Point==TEXT("truck")&&S->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Sound&&FMath::IsNearlyEqual(B.Delay,2.6)&&FMath::IsNearlyEqual(B.Rate,.9);}),TEXT("hood_warm: two ticks at the truck, the second slower after 2.6 s"));
 }

 // ---- N1110 (N1110-3): the camera moments' three, and the camera_snap beat ----
 for(const TCHAR* Id:{TEXT("eyes_window"),TEXT("beam_figure"),TEXT("dish_lights")})
 {
  const FS47EventScript* S=Built.Find(Id);
  const FS47Beat* C=S?First(*S,ES47Beat::CameraSnap):nullptr;
  const double Whole=C?C->In+C->Hold+C->Out:0;
  Check(S&&C&&CountOf(*S,ES47Beat::CameraSnap)==1&&!C->Point.IsEmpty()&&PointsOf(Id).Contains(C->Point)&&Whole>=.6&&Whole<=2.5&&C->Fov>=30&&C->Fov<=45&&!C->Kind.IsEmpty()&&CountOf(*S,ES47Beat::Evidence)==1&&S->Gates.Any(),
   FString::Printf(TEXT("%s: one snap of the view at its point, 0.6 to 2.5 s, to 30 to 45 degrees, with a sting, a finding and gates"),Id));
 }
 if(const FS47EventScript* S=Built.Find(TEXT("eyes_window")))
 {
  const FS47Beat* C=First(*S,ES47Beat::CameraSnap);
  Check(S->Gates.bTier&&S->Gates.Tier==ES47Allowed::Full&&S->Gates.HourFrom==2&&C&&C->Kind==TEXT("hard")&&FMath::IsNearlyEqual(C->Hold,1.2)&&C->Strength>0,TEXT("eyes_window: after two, in the full tier, a hard hit, held 1.2 s with a little shake"));
 }
 if(const FS47EventScript* S=Built.Find(TEXT("dish_lights")))
 {
  const FS47Beat* C=First(*S,ES47Beat::CameraSnap);
  Check(C&&FMath::IsNearlyEqual(C->At,4.2)&&C->Kind==TEXT("swell")&&S->Gates.Areas.Contains(TEXT("saro")),TEXT("dish_lights: the lights hold four seconds before the view goes up to them (N1110 critic); SARO only"));
 }
 if(const FS47EventScript* S=Built.Find(TEXT("beam_figure")))
 {
  const FS47Beat* C=First(*S,ES47Beat::CameraSnap);
  Check(C&&FMath::IsNearlyEqual(C->In,.3)&&C->Kind==TEXT("soft"),TEXT("beam_figure: a soft sting and the view in over 0.3 s (the code leaves the snap out before 02:00)"));
 }
 // ---- N1110 (SOUNDSCAPE, GEAR-FLICKER, PLAY-31-34): the silence beat before the sight, the gear answering, the tube beat ----
 for(const TCHAR* Id:{TEXT("eyes_window"),TEXT("beam_figure"),TEXT("dish_lights")})
 {
  const FS47EventScript* S=Built.Find(Id);
  const FS47Beat* Quiet=S?First(*S,ES47Beat::Silence):nullptr;const FS47Beat* C=S?First(*S,ES47Beat::CameraSnap):nullptr;const FS47Beat* Gear=S?First(*S,ES47Beat::Light):nullptr;
  Check(Quiet&&C&&Gear&&CountOf(*S,ES47Beat::Silence)==1&&Quiet->At==0&&Quiet->At+2.<C->At&&Quiet->Secs>=C->At&&Quiet->Strength<=.1,
   FString::Printf(TEXT("%s: the night goes quiet first, stays quiet through the sight, and the gear answers with it"),Id));
 }
 {
  FS47EventScript S;FString Why;
  const bool bRead=Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":0,\"do\":\"silence\",\"secs\":6,\"level\":0.03},{\"at\":1,\"do\":\"silence\",\"secs\":3},{\"at\":2,\"do\":\"tube\",\"tube\":3}]}"),S,Why);
  Check(bRead&&S.Beats.Num()==3&&S.Beats[0].Do==ES47Beat::Silence&&FMath::IsNearlyEqual(S.Beats[0].Strength,.03)&&FMath::IsNearlyEqual(S.Beats[1].Strength,.04)&&S.Beats[2].Do==ES47Beat::Tube&&S.Beats[2].Tube==3&&Lint(S).IsEmpty(),TEXT("silence and tube: read with their level (0.04 as made) and tube, and clean"));
  FS47EventScript T=S;
  if(bRead){T.Beats[0].Secs=0;Check(Lint(T).Num()>0,TEXT("silence: no length is refused"));
  T=S;T.Beats[0].Secs=99;Check(Lint(T).Num()>0,TEXT("silence: a minute and a half is refused"));
  T=S;T.Beats[2].Tube=1;Check(Lint(T).Num()>0,TEXT("tube: the working light (tube 1) is refused"));
  T=S;T.Beats[2].Tube=-1;Check(Lint(T).Num()>0,TEXT("tube: no tube is refused"));}
 }
 {
  // N1110: every sabotage (relay, mains, lamp, truck) puts out the next two working ceiling tubes, and each script is clean.
  bool bSabs=true,bFound=false;
  for(const TCHAR* Sab:{TEXT("relay"),TEXT("mains"),TEXT("lamp"),TEXT("truck")})
  {
   FString Text;FS47EventScript S;FString Why;
   if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Director/Events/attention_sab_")+Sab+TEXT(".json")))||!Read(Text,S,Why))continue;
   bFound=true;const FS47Beat* T=First(S,ES47Beat::Tube);
   bSabs&=T&&CountOf(S,ES47Beat::Tube)==1&&T->Next==2&&T->Tube<0&&Lint(S).IsEmpty();
  }
  if(bFound)Check(bSabs,TEXT("attention_sab_*: every sabotage puts out the next two working ceiling tubes (tube beat, next 2), and the scripts are clean"));
  FS47EventScript N;FString NoWhy;
  const bool bNext=Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":0,\"do\":\"tube\",\"next\":2}]}"),N,NoWhy);
  Check(bNext&&N.Beats.Num()==1&&N.Beats[0].Next==2&&N.Beats[0].Tube<0&&Lint(N).IsEmpty(),TEXT("tube beat: next 2 is read and clean"));
  if(bNext)
  {
   FS47EventScript T=N;T.Beats[0].Next=0;Check(Lint(T).Num()>0,TEXT("tube beat: neither tube nor next is refused"));
   T=N;T.Beats[0].Next=7;Check(Lint(T).Num()>0,TEXT("tube beat: next 7 is refused (six tubes can go)"));
   T=N;T.Beats[0].Tube=3;Check(Lint(T).Num()>0,TEXT("tube beat: tube and next together are refused"));
  }
 }
 {
  FS47EventScript S;FString Why;
  const bool bRead=Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":1,\"do\":\"camera_snap\",\"point\":\"eyes\",\"fov\":38,\"in\":0.2,\"hold\":1.2,\"out\":0.9,\"shake\":0.25,\"scare\":\"hard\"},{\"at\":2,\"do\":\"camera_snap\",\"pos\":[1,2,3]}]}"),S,Why);
  Check(bRead&&S.Beats.Num()==2&&S.Beats[0].Do==ES47Beat::CameraSnap&&S.Beats[0].Point==TEXT("eyes")&&S.Beats[0].Fov==38&&FMath::IsNearlyEqual(S.Beats[0].In,.2)&&FMath::IsNearlyEqual(S.Beats[0].Hold,1.2)&&FMath::IsNearlyEqual(S.Beats[0].Out,.9)&&FMath::IsNearlyEqual(S.Beats[0].Strength,.25)&&S.Beats[0].Kind==TEXT("hard"),TEXT("camera_snap: its point, degrees, seconds, shake and sting are read"));
  Check(bRead&&S.Beats[1].Pos.IsSet()&&S.Beats[1].Fov==45&&FMath::IsNearlyEqual(S.Beats[1].In,.25)&&FMath::IsNearlyEqual(S.Beats[1].Hold,1.2)&&FMath::IsNearlyEqual(S.Beats[1].Out,.9)&&S.Beats[1].Strength==0&&S.Beats[1].Kind.IsEmpty(),TEXT("camera_snap: as made, 45 degrees in .25 s, held 1.2 s, back in .9 s, no shake and no sting"));
  Check(bRead&&Lint(S).IsEmpty(),TEXT("camera_snap: both are clean"));
  FS47EventScript T=S;if(bRead){T.Beats[0].Fov=10;Check(Lint(T).Num()>0,TEXT("camera_snap: 10 degrees is refused"));
  T=S;T.Beats[0].Hold=6;Check(Lint(T).Num()>0,TEXT("camera_snap: six seconds held is refused"));
  T=S;T.Beats[0].Kind=TEXT("loud");Check(Lint(T).Num()>0,TEXT("camera_snap: an unknown sting is refused"));
  T=S;T.Beats[0].Point.Reset();Check(Lint(T).Num()>0,TEXT("camera_snap: without a point or a pos it is refused"));
  T=S;T.Beats[0].In=0;Check(Lint(T).Num()>0,TEXT("camera_snap: no way in is refused"));}
 }

 // ---- N1110-5: four more happenings in the script layer (horizon_figure, figure_near, landing, car_light) ----
 {
  const auto Evid=[&Built](const TCHAR* Id,const TCHAR* Key,const TCHAR* Part)
  {
   const FS47EventScript* S=Built.Find(Id);const FS47Beat* E=S?First(*S,ES47Beat::Evidence):nullptr;
   return S&&E&&CountOf(*S,ES47Beat::Evidence)==1&&E->At==0&&E->Key==Key&&E->Text.Contains(Part);
  };
  Check(Evid(TEXT("horizon_figure_gone"),TEXT("horizon_figure_d{day}"),TEXT("stood on the rise to the {dir}, a long way off, dead still")),TEXT("horizon_figure_gone: the finding the code kept, with the direction"));
  Check(Evid(TEXT("figure_near_end"),TEXT("figure_near_d{day}"),TEXT("out on {place}: {seen}{end}")),TEXT("figure_near_end: the finding the code kept, built from what was seen and how it ended"));
  Check(Evid(TEXT("landing_seen_log"),TEXT("landing_seen_d{landing_night}"),TEXT("{az} degrees from SARO, give or take {spread}, and went out on the ground")),TEXT("landing_seen_log: the finding the code kept (the ledger and the same-minute check read its key)"));
  Check(Evid(TEXT("landing_logged"),TEXT("landing_logged_d{landing_night}"),TEXT("the array logged something low on the horizon coming down")),TEXT("landing_logged: the array's finding the code kept"));
  Check(Evid(TEXT("car_light_end"),TEXT("car_light_d{day}"),TEXT("the engine died under it.{outcome}")),TEXT("car_light_end: the finding the code kept, with what the player did"));
  // the quiet before: a start script opens with the night going still, and nothing else comes before it
  for(const TCHAR* Id:{TEXT("horizon_figure"),TEXT("car_light")})
  {
   const FS47EventScript* S=Built.Find(Id);
   Check(S&&S->Beats.Num()>=3&&S->Beats[0].Do==ES47Beat::Silence&&S->Beats[0].At==0&&CountOf(*S,ES47Beat::Silence)==1&&CountOf(*S,ES47Beat::Tone)==1&&First(*S,ES47Beat::Light)&&S->Beats[0].Strength<=.1,
    FString::Printf(TEXT("%s: the night goes still first, then a low hum, and the gear answers"),Id));
  }
  // steps and a step: heard at the figure's point
  if(const FS47EventScript* S=Built.Find(TEXT("figure_near_steps")))
  {
   int32 Steps=0;double Last=-1;bool bAt=true;
   for(const FS47Beat& B:S->Beats)if(B.Do==ES47Beat::Sound){++Steps;bAt&=B.Point==TEXT("figure")&&B.Name.StartsWith(TEXT("stepGravel"))&&B.Delay>Last;Last=B.Delay;}
   Check(Steps==3&&bAt&&FMath::IsNearlyEqual(Last,1.1),TEXT("figure_near_steps: three steps in the gravel at the figure, .55 s apart"));
  }
  if(const FS47EventScript* S=Built.Find(TEXT("figure_near_close")))
  {
   const FS47Beat* Step=First(*S,ES47Beat::Sound);const FS47Beat* C=First(*S,ES47Beat::CameraSnap);
   Check(Step&&C&&CountOf(*S,ES47Beat::Sound)==1&&Step->At<C->At&&C->Point==TEXT("figure")&&C->Kind==TEXT("soft")&&C->In+C->Hold+C->Out<=2.5,TEXT("figure_near_close: one step, then the view turns to it (a soft sting, under 2.5 s)"));
  }
  // the view is taken only where it is earned: one place in all of them, and the code leaves it out before 02:00
  int32 Snaps=0;
  for(const TCHAR* Id:{TEXT("horizon_figure"),TEXT("horizon_figure_gone"),TEXT("figure_near"),TEXT("figure_near_steps"),TEXT("figure_near_close"),TEXT("figure_near_light"),TEXT("figure_near_end"),TEXT("landing_warning"),TEXT("landing_seen_log"),TEXT("landing_logged"),TEXT("car_light"),TEXT("car_light_end")})
   if(const FS47EventScript* S=Built.Find(Id))Snaps+=CountOf(*S,ES47Beat::CameraSnap);
  Check(Snaps==1,TEXT("N1110-5: the camera is taken in one place only (figure_near_close, after 02:00)"));
  if(const FS47EventScript* S=Built.Find(TEXT("landing_logged")))
  {
   const FS47Beat* Beep=First(*S,ES47Beat::Tone);const FS47Beat* Pr=First(*S,ES47Beat::Printer);const FS47Beat* To=First(*S,ES47Beat::Toast);
   Check(Beep&&Pr&&To&&Beep->Repeat==4&&FMath::IsNearlyEqual(Beep->Hz,1320.)&&FMath::IsNearlyEqual(Beep->Every,.18)&&FMath::IsNearlyEqual(Pr->Delay,.9)&&FMath::IsNearlyEqual(Pr->Stop,3.)&&Beep->If.Num()==1&&Pr->If.Num()==1&&To->If.Num()==1&&To->Text.Contains(TEXT("{map_key}")),
    TEXT("landing_logged: four beeps, the printer from .9 s and the line on the screen, for one in the building, as the code did"));
  }
  if(const FS47EventScript* S=Built.Find(TEXT("landing_warning")))
  {
   const FS47Beat* Dale=First(*S,ES47Beat::Line);
   Check(Dale&&Dale->Who==TEXT("dale")&&Dale->At>=6&&CountOf(*S,ES47Beat::Thought)==2&&CountOf(*S,ES47Beat::Tone)==1&&First(*S,ES47Beat::Tone)->At==0,TEXT("landing_warning: a hum at once, a thought by where the player is, and Dale on the walkie after the power falters"));
  }
  // the gates stay where they were: the rhythm and the code's own (a start script has none of its own, nor any of the stages)
  bool bNoGates=true;
  for(const TCHAR* Id:{TEXT("horizon_figure"),TEXT("horizon_figure_gone"),TEXT("figure_near"),TEXT("figure_near_steps"),TEXT("figure_near_close"),TEXT("figure_near_light"),TEXT("figure_near_end"),TEXT("landing_warning"),TEXT("landing_seen_log"),TEXT("landing_logged"),TEXT("car_light"),TEXT("car_light_end")})
   if(const FS47EventScript* S=Built.Find(Id))bNoGates&=!S->Gates.Any()&&S->bEnabled;
  Check(bNoGates,TEXT("N1110-5: the twelve scripts add no gates of their own (the rhythm, the tiers and the code's rules are as they were)"));
 }

 // ---- every file it reads: parses, known beats, clean; a wired one as it is built in, or said ----
 int32 Files=0;
 for(const FString& Dir:ReadDirs())
 {
  TArray<FString> Names;
  IFileManager::Get().FindFiles(Names,*(Dir/TEXT("*.json")),true,false);
  for(const FString& Name:Names)
  {
   FString Text,Why;FS47EventScript S;
   ++Files;
   if(!FFileHelper::LoadFileToString(Text,*(Dir/Name))){Check(false,Name+TEXT(" could not be read"));continue;}
   const bool bRead=Read(Text,S,Why);
   const TArray<FString> Errors=bRead?Lint(S):TArray<FString>();
   Check(bRead&&Errors.IsEmpty()&&S.Id+TEXT(".json")==Name,FString::Printf(TEXT("%s parses, names only known beats, keeps to the rules and is named by its id%s"),*Name,!bRead?*(TEXT(": ")+Why):Errors.Num()?*(TEXT(": ")+Errors[0]):TEXT("")));
   for(const auto& P:BuiltIn())
    if(P.Key==S.Id&&Text.Replace(TEXT("\r"),TEXT("")).TrimStartAndEnd()!=FString(P.Value).Replace(TEXT("\r"),TEXT("")).TrimStartAndEnd())
     Lines.Add(FString::Printf(TEXT("NOTE event scripts: %s differs from the built-in copy (edited: the file plays; S47EventScriptDefaults.inl keeps the old beats)"),*Name));
  }
 }
 if(!Files)Lines.Add(TEXT("NOTE event scripts: no files beside this build (Content/Director/Events): the built-in copies play"));
 const FString Example=FPaths::ProjectContentDir()/TEXT("Director/Events/example_every_beat.json");
 FString ExampleText;
 if(FFileHelper::LoadFileToString(ExampleText,*Example))
 {
  FS47EventScript S;FString Why;
  bool bEvery=Read(ExampleText,S,Why)&&!S.bEnabled;
  for(const TCHAR* B:{TEXT("sound"),TEXT("tone"),TEXT("printer"),TEXT("light"),TEXT("tv"),TEXT("line"),TEXT("thought"),TEXT("toast"),TEXT("evidence"),TEXT("shake"),TEXT("camera_snap"),TEXT("silence"),TEXT("tube")}){ES47Beat Do=ES47Beat::Count;bEvery&=BeatFrom(B,Do)&&CountOf(S,Do)>0;}
  Check(bEvery&&S.Gates.Any(),TEXT("the example shows the beats and the gates, and is switched off"));
 }

 // ---- reading: what is not known is refused by name; the order of the clock ----
 {
  FS47EventScript S;FString Why;
  Check(!Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":0,\"do\":\"explode\"}]}"),S,Why)&&Why.Contains(TEXT("explode")),TEXT("an unknown beat is refused by name"));
  Check(!Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":0,\"do\":\"toast\",\"text\":\"a\",\"if\":[\"upside_down\"]}]}"),S,Why)&&Why.Contains(TEXT("upside_down")),TEXT("an unknown condition is refused by name"));
  Check(!Read(TEXT("{\"version\":2,\"id\":\"x\"}"),S,Why)&&!Read(TEXT("not json"),S,Why)&&!Read(TEXT("{\"version\":1,\"id\":\"x\",\"gates\":{\"tier\":\"soon\"}}"),S,Why),TEXT("another version, broken text or an unknown tier is refused"));
  const bool bRead=Read(TEXT("{\"version\":1,\"id\":\"x\",\"beats\":[{\"at\":5,\"do\":\"toast\",\"text\":\"b\"},{\"at\":1,\"do\":\"toast\",\"text\":\"a\"},{\"at\":5,\"do\":\"thought\",\"text\":\"c\",\"wait\":[{\"if\":\"area\",\"is\":\"saro\"}],\"max\":9}]}"),S,Why);
  Check(bRead&&S.Beats.Num()==3&&S.Beats[0].Text==TEXT("a")&&S.Beats[1].Text==TEXT("b")&&S.Beats[2].Do==ES47Beat::Thought&&S.Beats[2].Wait.Num()==1&&S.Beats[2].Wait[0].Arg==TEXT("saro"),TEXT("beats in the order of the clock, the same second as written; a wait keeps its condition"));
 }

 // ---- the lint ----
 {
  FS47EventScript S;S.Id=TEXT("lint_test");
  FS47Beat B;B.Do=ES47Beat::Thought;B.Text=TEXT("No. Not again.");S.Beats.Add(B);
  TArray<FString> Warn;
  Check(Lint(S,&Warn).IsEmpty()&&Warn.ContainsByPredicate([](const FString& W){return W.Contains(TEXT("F10 only"));}),TEXT("a clean script for no event is clean, and said to run from F10 only"));
  for(const TCHAR* Bad:{TEXT("They were taken."),TEXT("The radio said 1947."),TEXT("Forty seven seconds on the tape.")})
  {
   FS47EventScript T=S;T.Beats[0].Text=Bad;
   Check(Lint(T).Num()>0,FString::Printf(TEXT("refused: \"%s\""),Bad));
  }
  FS47EventScript T=S;T.Beats[0].Do=ES47Beat::Line;T.Beats[0].Who=TEXT("ghost");
  Check(Lint(T).Num()>0,TEXT("a line needs dale, voice, cb or radio"));
  T=S;T.Beats[0].Wait.Add(FS47ScriptCond());T.Beats[0].Max=0;
  Check(Lint(T).Num()>0,TEXT("a wait needs its most seconds"));
  T=S;T.Id=TEXT("array_burst");T.Beats[0].Text=TEXT("About {azimuth} degrees.");Warn.Reset();Lint(T,&Warn);
  Check(Warn.ContainsByPredicate([](const FString& W){return W.Contains(TEXT("{azimuth}"));}),TEXT("a word the event does not give is warned about"));
 }

 // ---- the words ----
 {
  const TMap<FString,FString> W={{TEXT("az"),TEXT("120")},{TEXT("clock"),TEXT("22:40")}};
  Check(Fill(TEXT("At {clock}, about {az} degrees ({az}); {nothing}."),W)==TEXT("At 22:40, about 120 degrees (120); {nothing}."),TEXT("words filled in, an unknown one left as written"));
 }

 // ---- N1110 (pace, critic 2) ----
 {
  // the odds roll a script is asked with is seeded by the seed, the night and the script: the same every time that night is asked
  const uint32 Salt=GetTypeHash(FString(TEXT("animals_warn_birds")))^0x45565453u;
  bool bSame=true;int32 Yes=0;
  for(int32 N=1;N<=200;++N){const float A=FS47NightRhythm::Roll(47,N,Salt);bSame&=A==FS47NightRhythm::Roll(47,N,Salt);Yes+=A<.25f;}
  Check(bSame&&Yes>25&&Yes<75,TEXT("a script's odds are rolled once per night (seeded), so .25 means about a quarter of the nights"));
  for(const TCHAR* Id:{TEXT("animals_warn_birds"),TEXT("animals_warn_coyote")})
  {
   const FS47EventScript* W=Find(Id);
   Check(W&&W->Gates.DayFrom==4,FString::Printf(TEXT("%s: from the fourth night"),Id));
  }
  const FS47EventScript* Nt=Find(TEXT("attention_hint_dale_neutral"));
  Check(Nt&&Nt->bEnabled&&Lint(*Nt).Num()==0,TEXT("attention_hint_dale_neutral: there, enabled and clean"));
  Check(FS47FreeformSession::LessonRecordKey(TEXT("attention_rule"))&&FS47FreeformSession::LessonRecordKey(TEXT("rule-still-held"))&&FS47FreeformSession::LessonRecordKey(TEXT("rule-pale-broken"))&&!FS47FreeformSession::LessonRecordKey(TEXT("attention_rule_still"))&&!FS47FreeformSession::LessonRecordKey(TEXT("place_windmill")),TEXT("the stare rule and the still rule's records are the lessons that pay a field record"));
 }
 // ---- the gates ----
 {
  FS47EventGates G;FString Why;
  Check(!G.Any()&&GatesHold(G,At(1,21),TEXT("saro"),.99,Why),TEXT("no gates: it always may"));
  G.DayFrom=3;G.HourFrom=22;G.HourTo=4;
  Check(!GatesHold(G,At(2,23),TEXT("saro"),0,Why)&&GatesHold(G,At(3,23),TEXT("saro"),0,Why)&&GatesHold(G,At(3,3),TEXT("saro"),0,Why)&&!GatesHold(G,At(3,21),TEXT("saro"),0,Why),TEXT("days and hours past midnight"));
  G=FS47EventGates();G.bTier=true;G.Tier=ES47Allowed::Full;
  Check(!GatesHold(G,At(5,23.5),TEXT("road"),0,Why)&&GatesHold(G,At(5,2.5),TEXT("road"),0,Why)&&GatesHold(G,At(22,1.5),TEXT("road"),0,Why),TEXT("a full tier waits for two, one in the fourth week"));
  G.bEarly=true;
  Check(GatesHold(G,At(5,23.5),TEXT("road"),0,Why),TEXT("early, on purpose, lets it come sooner"));
  G=FS47EventGates();G.Areas={TEXT("saro")};G.Odds=.4;
  Check(GatesHold(G,At(5,23),TEXT("saro"),.39,Why)&&!GatesHold(G,At(5,23),TEXT("saro"),.4,Why)&&!GatesHold(G,At(5,23),TEXT("road"),.1,Why),TEXT("areas and odds"));
  G=FS47EventGates();FS47ScriptCond C;C.If=TEXT("outdoors");G.If.Add(C);
  Check(!GatesHold(G,At(5,23),TEXT("saro"),0,Why)&&Why.Contains(TEXT("outdoors")),TEXT("a condition holds it back, and says which"));
  G.If[0].If=TEXT("area");G.If[0].Arg=TEXT("saro");
  Check(GatesHold(G,At(5,23),TEXT("saro"),0,Why,[](const FS47ScriptCond& X){return X.Arg==TEXT("saro");})&&!GatesHold(G,At(5,23),TEXT("saro"),0,Why),TEXT("its own conditions are asked of the game, and fail without it"));
 }
 return bAll;
}
