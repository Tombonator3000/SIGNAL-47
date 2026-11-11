#include "S47EventScript.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47CameraMoment.h"
#include "S47DevEvents.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47Omen.h"
#include "S47Places.h"
#include "S47Radio.h"
#include "S47ScriptPlayer.h"
#include "S47StoryDirector.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47EventScript,Log,All);

// D46-20261110 (D46-11): the event scripts' data, library, gates and runner (S47EventScript.h). Tests: S47EventScriptTest.cpp.

#include "S47EventScriptDefaults.inl"

namespace S47EventScriptLocal
{
 const TCHAR* const Names[int32(ES47Beat::Count)]={TEXT("sound"),TEXT("tone"),TEXT("printer"),TEXT("light"),TEXT("tv"),TEXT("line"),TEXT("thought"),TEXT("toast"),TEXT("evidence"),TEXT("map"),TEXT("prop"),TEXT("show"),TEXT("hide"),TEXT("shake"),TEXT("camera_snap"),TEXT("silence"),TEXT("tube"),TEXT("flag"),TEXT("start"),TEXT("clock"),TEXT("objective")};
 // Besides the scripted events' conditions (S47Script::CondKinds): in the SARO building, and the scene's area by name.
 // STORY-ON-FREEFORM-01 (step 4): and the story's flags, read by every script but let through only by the story store's lint.
 bool OwnCond(const FString& If){return If==TEXT("in_saro")||If==TEXT("area")||S47EventScript::StoryOnlyCond(If);}
 bool CondKnown(const FString& If){return OwnCond(If)||S47Script::CondKinds().Contains(If);}
 // The printer at the rack, where the night's events have always run it (scene metres at SARO).
 const FVector PrinterAt(-4,1,2);
 const TCHAR* const LampFrom=TEXT("eventscript");
 constexpr int32 MostRuns=8;
 constexpr double MostSeconds=600;

 // A condition: "outdoors", or { "if": "weather", "is": "clear" } with the scripted events' own fields.
 bool CondFrom(const TSharedPtr<FJsonValue>& V,FS47ScriptCond& C,FString& Why)
 {
  C=FS47ScriptCond();
  FString Word;
  const S47J::FObj* J=nullptr;
  if(V.IsValid()&&V->TryGetString(Word))C.If=Word;
  else if(V.IsValid()&&V->TryGetObject(J)&&J)
  {
   C.If=S47J::Str(*J,TEXT("if"));
   if(C.If==TEXT("finding")||C.If==TEXT("no_finding"))C.Arg=S47J::Str(*J,TEXT("id"));
   else if(C.If.StartsWith(TEXT("case_")))C.Num=S47J::Num(*J,TEXT("case"),-1);
   else if(C.If==TEXT("weather")||C.If==TEXT("area"))C.Arg=S47J::Str(*J,TEXT("is"));
   else if(C.If==TEXT("moon_min")||C.If==TEXT("moon_max"))C.Num=S47J::Num(*J,TEXT("lit"));
   else if(C.If==TEXT("near")||C.If==TEXT("far")){C.Arg=S47J::Str(*J,TEXT("place"));C.Num=S47J::Num(*J,TEXT("m"),100);}
   else if(S47EventScript::StoryOnlyCond(C.If))C.Arg=S47J::Str(*J,TEXT("flag")); // STORY-ON-FREEFORM-01 (step 4)
  }
  if(!CondKnown(C.If)){Why=FString::Printf(TEXT("condition \"%s\" is not known"),*C.If);return false;}
  return true;
 }
 bool CondsFrom(const S47J::FObj& O,const TCHAR* K,TArray<FS47ScriptCond>& Out,FString& Why)
 {
  for(const TSharedPtr<FJsonValue>& V:S47J::Arr(O,K)){FS47ScriptCond C;if(!CondFrom(V,C,Why))return false;Out.Add(C);}
  return true;
 }
 bool VecFrom(const S47J::FObj& O,const TCHAR* K,FVector& Out)
 {
  const TArray<double> V=S47J::Nums(O,K);
  if(V.Num()!=3||!FMath::IsFinite(V[0])||!FMath::IsFinite(V[1])||!FMath::IsFinite(V[2]))return false;
  Out=FVector(V[0],V[1],V[2]);return true;
 }
 // An hour as a number (23.5) or as text ("23:30").
 double HourFrom(const TSharedPtr<FJsonValue>& V)
 {
  double D=-1;FString S;
  if(V.IsValid()&&V->TryGetNumber(D))return FMath::IsFinite(D)?FMath::Fmod(FMath::Max(0.,D),24.):-1;
  if(V.IsValid()&&V->TryGetString(S))return S47Script::ParseHour(S);
  return -1;
 }
 bool BeatOf(const S47J::FObj& J,FS47Beat& B,FString& Why)
 {
  const FString Do=S47J::Str(J,TEXT("do"));
  if(!S47EventScript::BeatFrom(Do,B.Do)){Why=FString::Printf(TEXT("beat \"%s\" is not known"),*Do);return false;}
  B.At=S47J::Num(J,TEXT("at"));
  B.Name=B.Do==ES47Beat::Sound?S47J::Str(J,TEXT("sound")):B.Do==ES47Beat::Map?S47J::Str(J,TEXT("place")):S47J::Str(J,TEXT("node"));
  B.Kind=B.Do==ES47Beat::Tone?S47J::Str(J,TEXT("wave"),TEXT("square")):S47J::Str(J,TEXT("kind"));
  B.Who=S47J::Str(J,TEXT("who"));B.Text=S47J::Str(J,TEXT("text"));B.Key=S47J::Str(J,TEXT("key"));B.Point=S47J::Str(J,TEXT("point"));
  FVector V;
  if(VecFrom(J,TEXT("pos"),V))B.Pos=V;
  if(VecFrom(J,TEXT("by"),V))B.By=V;
  B.bTo=S47J::Bool(J,TEXT("to"));
  // The printer's and a tone's own levels, as the events had them.
  B.Gain=S47J::Num(J,TEXT("gain"),B.Do==ES47Beat::Printer?.3:B.Do==ES47Beat::Tone?.12:1.);
  B.Rate=S47J::Num(J,TEXT("rate"),1);B.Lp=S47J::Num(J,TEXT("lp"));B.Delay=S47J::Num(J,TEXT("delay"));B.Stop=S47J::Num(J,TEXT("stop"));
  B.Secs=S47J::Num(J,TEXT("secs"));B.Hz=S47J::Num(J,TEXT("hz"),1200);B.Every=S47J::Num(J,TEXT("every"));
  B.Strength=S47J::Num(J,TEXT("strength"),.5);B.Max=S47J::Num(J,TEXT("max"));B.Repeat=S47J::Int(J,TEXT("repeat"),1);
  // N1110: camera_snap's own words: fov, in, hold, out, shake (its strength, none unless asked) and scare (its kind).
  if(B.Do==ES47Beat::CameraSnap)
  {
   B.Fov=S47J::Num(J,TEXT("fov"),45);B.In=S47J::Num(J,TEXT("in"),.25);B.Hold=S47J::Num(J,TEXT("hold"),1.2);B.Out=S47J::Num(J,TEXT("out"),.9);
   B.Strength=S47J::Num(J,TEXT("shake"),0);B.Kind=S47J::Str(J,TEXT("scare"));
  }
  // N1110: silence's level (what is left of the night's sound, as strength) and the tube beat's tube.
  if(B.Do==ES47Beat::Silence)B.Strength=S47J::Num(J,TEXT("level"),.04);
  if(B.Do==ES47Beat::Tube){B.Tube=S47J::Int(J,TEXT("tube"),-1);B.Next=S47J::Int(J,TEXT("next"),0);}
  // STORY-ON-FREEFORM-01 (step 4): the story-only beats' own words.
  if(B.Do==ES47Beat::Flag){B.Flag=S47J::Str(J,TEXT("flag"));B.bSet=S47J::Bool(J,TEXT("set"),true);}
  if(B.Do==ES47Beat::Start){B.Script=S47J::Str(J,TEXT("script"));B.Happening=S47J::Str(J,TEXT("happening"));}
  if(B.Do==ES47Beat::Clock&&!FS47StoryDirector::ParseFrom(S47J::Str(J,TEXT("at_least")),B.AtLeast))B.AtLeast=-1;
  return CondsFrom(J,TEXT("if"),B.If,Why)&&CondsFrom(J,TEXT("wait"),B.Wait,Why);
 }
 // The words a text asks for: {az} -> az.
 TArray<FString> WordsIn(const FString& Text)
 {
  TArray<FString> Out;
  int32 From=0;
  while(true)
  {
   const int32 Open=Text.Find(TEXT("{"),ESearchCase::CaseSensitive,ESearchDir::FromStart,From);if(Open<0)break;
   const int32 Close=Text.Find(TEXT("}"),ESearchCase::CaseSensitive,ESearchDir::FromStart,Open+1);if(Close<0)break;
   Out.AddUnique(Text.Mid(Open+1,Close-Open-1));From=Close+1;
  }
  return Out;
 }
 // The player's words: the spoiler lint, and never the word for being carried off.
 FString PlayerWords(const FString& Text)
 {
  const FString Why=S47Script::LintText(Text);
  if(!Why.IsEmpty())return Why;
  TArray<FString> Parts;Text.ToLower().ParseIntoArray(Parts,TEXT(" "));
  for(FString P:Parts){P=P.TrimStartAndEnd();while(P.Len()&&!FChar::IsAlpha(P[P.Len()-1]))P.LeftChopInline(1);while(P.Len()&&!FChar::IsAlpha(P[0]))P.RightChopInline(1);if(P==TEXT("taken"))return TEXT("the word \"taken\" is never said to the player");}
  return FString();
 }

 struct FLib
 {
  TArray<FS47EventScript> Scripts;
  TMap<FString,FString> Bad;
  bool bRead=false;
 };
 FLib& Lib(){static FLib L;return L;}
 void ReadOnce(){if(!Lib().bRead)S47EventScript::Reload();}
}

namespace S47EventScript
{
const TCHAR* BeatName(ES47Beat B){return int32(B)<int32(ES47Beat::Count)?S47EventScriptLocal::Names[int32(B)]:TEXT("?");}
bool BeatFrom(const FString& Name,ES47Beat& Out)
{
 for(int32 I=0;I<int32(ES47Beat::Count);++I)if(Name==S47EventScriptLocal::Names[I]){Out=ES47Beat(I);return true;}
 return false;
}
const TArray<FString>& BeatNames()
{
 static const TArray<FString> L=[]{TArray<FString> Out;for(int32 I=0;I<int32(ES47Beat::Count);++I)Out.Add(S47EventScriptLocal::Names[I]);return Out;}();
 return L;
}

bool StoryOnlyBeat(ES47Beat B){return B==ES47Beat::Flag||B==ES47Beat::Start||B==ES47Beat::Clock||B==ES47Beat::Objective;}
bool StoryOnlyCond(const FString& If){return If==TEXT("story_flag")||If==TEXT("no_story_flag");}

bool Read(const FString& Text,FS47EventScript& Out,FString& Why)
{
 using namespace S47EventScriptLocal;
 Out=FS47EventScript();
 S47J::FObj O;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O)||!O.IsValid()){Why=TEXT("not JSON");return false;}
 if(S47J::Int(O,TEXT("version"),0)!=FS47EventScript::Version){Why=FString::Printf(TEXT("not version %d"),FS47EventScript::Version);return false;}
 Out.Id=S47J::Str(O,TEXT("id"));Out.Name=S47J::Str(O,TEXT("name"));Out.Notes=S47J::Str(O,TEXT("notes"));
 Out.bEnabled=S47J::Bool(O,TEXT("enabled"),true);
 if(Out.Id.IsEmpty()){Why=TEXT("no id");return false;}
 const S47J::FObj G=S47J::Obj(O,TEXT("gates"));
 if(G.IsValid())
 {
  FS47EventGates& Gt=Out.Gates;
  Gt.DayFrom=FMath::Max(0,S47J::Int(G,TEXT("from_day")));Gt.DayTo=FMath::Max(0,S47J::Int(G,TEXT("to_day")));
  const TArray<TSharedPtr<FJsonValue>> H=S47J::Arr(G,TEXT("hours"));
  if(H.Num()==2){Gt.HourFrom=HourFrom(H[0]);Gt.HourTo=HourFrom(H[1]);}
  else if(H.Num()){Why=TEXT("gates: hours is [from, to]");return false;}
  if(S47J::Has(G,TEXT("tier")))
  {
   if(!S47NightTiers::FromName(S47J::Str(G,TEXT("tier")),Gt.Tier)){Why=TEXT("gates: tier is hint, glimpse or full");return false;}
   Gt.bTier=true;
  }
  Gt.bEarly=S47J::Bool(G,TEXT("early"));
  Gt.Areas=S47J::Strs(G,TEXT("areas"));
  Gt.Odds=S47J::Num(G,TEXT("odds"),1);
  if(!CondsFrom(G,TEXT("if"),Gt.If,Why)){Why=TEXT("gates: ")+Why;return false;}
 }
 const TArray<TSharedPtr<FJsonValue>> Beats=S47J::Arr(O,TEXT("beats"));
 for(int32 I=0;I<Beats.Num();++I)
 {
  const S47J::FObj* J=nullptr;
  if(!Beats[I].IsValid()||!Beats[I]->TryGetObject(J)||!J){Why=FString::Printf(TEXT("beat %d is not an object"),I+1);return false;}
  FS47Beat B;
  if(!BeatOf(*J,B,Why)){Why=FString::Printf(TEXT("beat %d: %s"),I+1,*Why);return false;}
  Out.Beats.Add(B);
 }
 // The order of the clock; the same second keeps the order written.
 Out.Beats.StableSort([](const FS47Beat& X,const FS47Beat& Y){return X.At<Y.At;});
 return true;
}

// Freeform's (bStory false) and the story store's (true) lint; see Lint and LintStory in the header.
static TArray<FString> LintAs(const FS47EventScript& S,TArray<FString>* Warnings,bool bStory)
{
 using namespace S47EventScriptLocal;
 TArray<FString> E;
 const auto Warn=[Warnings](const FString& W){if(Warnings)Warnings->Add(W);};
 // STORY-ON-FREEFORM-01 (step 4): the story's conditions, refused in Freeform's folders and checked for a flag's id in the story's.
 const auto CondsLint=[&E,bStory](const TArray<FS47ScriptCond>& Conds,const FString& Where)
 {
  for(const FS47ScriptCond& C:Conds)
  {
   if(!StoryOnlyCond(C.If))continue;
   if(!bStory)E.Add(FString::Printf(TEXT("%s: condition %s is story-only (Content/Director/Story/beats)"),*Where,*C.If));
   else if(!FS47StoryDirector::ValidId(C.Arg))E.Add(FString::Printf(TEXT("%s: %s needs flag, a story flag's id"),*Where,*C.If));
  }
 };
 CondsLint(S.Gates.If,TEXT("gates"));
 if(S.Id.IsEmpty()||S47Script::CleanId(S.Id)!=S.Id)E.Add(TEXT("id: a to z, 0 to 9 and _ only"));
 const FS47EventGates& G=S.Gates;
 if(G.DayTo>0&&G.DayFrom>G.DayTo)E.Add(TEXT("gates: the days run backwards"));
 if((G.HourFrom<0)!=(G.HourTo<0))E.Add(TEXT("gates: an hour window needs both ends"));
 if(!(G.Odds>=0&&G.Odds<=1))E.Add(TEXT("gates: odds are 0 to 1"));
 for(const FString& A:G.Areas)if(A.IsEmpty())E.Add(TEXT("gates: an empty area"));
 if(G.Odds<=0)Warn(TEXT("gates: odds 0, it never starts"));
 const bool bWired=Wired().Contains(S.Id);
 if(!bWired&&!bStory)Warn(TEXT("no event's code plays this one: it runs from F10 only"));
 if(!S.bEnabled&&bWired)Warn(TEXT("switched off: the built-in beats play instead"));
 TArray<FString> Words={TEXT("clock"),TEXT("day"),TEXT("night"),TEXT("map_key")};Words.Append(WordsOf(S.Id));
 TArray<FString> Points={TEXT("here")};Points.Append(PointsOf(S.Id));
 for(int32 I=0;I<S.Beats.Num();++I)
 {
  const FS47Beat& B=S.Beats[I];
  const FString At=FString::Printf(TEXT("beat %d (%s)"),I+1,BeatName(B.Do));
  const auto Need=[&E,&At](bool bOk,const TCHAR* What){if(!bOk)E.Add(At+TEXT(": ")+What);};
  CondsLint(B.If,At);CondsLint(B.Wait,At);
  if(!bStory&&StoryOnlyBeat(B.Do)){E.Add(At+TEXT(": a story-only beat (Content/Director/Story/beats); Freeform's events cannot play it"));continue;}
  Need(B.At>=0&&B.At<=MostSeconds,TEXT("at is 0 to 600 seconds"));
  Need(B.Gain>=0&&B.Gain<=2,TEXT("gain is 0 to 2"));
  Need(B.Delay>=0&&B.Delay<=30,TEXT("delay is 0 to 30 seconds"));
  Need(B.Stop>=0&&B.Stop<=MostSeconds&&B.Secs>=0&&B.Secs<=MostSeconds,TEXT("stop and secs are 0 to 600 seconds"));
  Need(B.Strength>=0&&B.Strength<=1,TEXT("strength is 0 to 1"));
  Need(B.Wait.IsEmpty()||B.Max>0,TEXT("a wait needs max, the most seconds it waits"));
  switch(B.Do)
  {
  case ES47Beat::Sound:Need(!B.Name.IsEmpty(),TEXT("needs sound, the recording's name"));Need(B.Who.IsEmpty()||B.Who==TEXT("radio")||B.Who==TEXT("cb")||B.Who==TEXT("walkie"),TEXT("who is radio, cb or walkie"));Need(B.Rate>=.25&&B.Rate<=4,TEXT("rate is 0.25 to 4"));break;
  case ES47Beat::Tone:Need(B.Hz>=20&&B.Hz<=8000,TEXT("hz is 20 to 8000"));Need(B.Repeat>=1&&B.Repeat<=32,TEXT("repeat is 1 to 32"));Need(B.Kind==TEXT("square")||B.Kind==TEXT("sine")||B.Kind==TEXT("sawtooth")||B.Kind==TEXT("triangle"),TEXT("wave is square, sine, sawtooth or triangle"));break;
  case ES47Beat::Light:Need(B.Kind==TEXT("town")||B.Kind==TEXT("lamp")||B.Kind==TEXT("gear"),TEXT("kind is town, lamp or gear"));break;
  case ES47Beat::Line:Need(B.Who==TEXT("dale")||B.Who==TEXT("voice")||B.Who==TEXT("cb")||B.Who==TEXT("radio"),TEXT("who is dale, voice, cb or radio"));Need(!B.Text.IsEmpty(),TEXT("needs text"));break;
  case ES47Beat::Thought:case ES47Beat::Toast:Need(!B.Text.IsEmpty(),TEXT("needs text"));break;
  case ES47Beat::Evidence:Need(!B.Key.IsEmpty()&&!B.Text.IsEmpty(),TEXT("needs key and text"));break;
  case ES47Beat::Map:Need(!B.Name.IsEmpty(),TEXT("needs place"));break;
  case ES47Beat::Prop:case ES47Beat::Show:case ES47Beat::Hide:Need(!B.Name.IsEmpty(),TEXT("needs node, the scene's key"));break;
  case ES47Beat::CameraSnap: // N1110
   Need(B.Pos.IsSet()||!B.Point.IsEmpty(),TEXT("needs pos or point, what it looks at"));
   Need(B.Fov>=S47CameraMoment::FovMin&&B.Fov<=S47CameraMoment::FovMax,TEXT("fov is 20 to 90 degrees"));
   Need(B.In>0&&B.Out>0&&B.Hold>=0&&B.In+B.Hold+B.Out>=S47CameraMoment::LeastSeconds&&B.In+B.Hold+B.Out<=S47CameraMoment::MostSeconds,TEXT("in and out above 0, hold 0 or more, in + hold + out 0.3 to 5 seconds"));
   Need(B.Kind.IsEmpty()||B.Kind==TEXT("soft")||B.Kind==TEXT("hard")||B.Kind==TEXT("braam")||B.Kind==TEXT("swell"),TEXT("scare is soft, hard, braam or swell"));
   break;
  case ES47Beat::Silence:Need(B.Secs>=.5&&B.Secs<=30,TEXT("needs secs, 0.5 to 30: how long the night holds still"));break; // N1110 (level is checked as strength)
  case ES47Beat::Flag:Need(FS47StoryDirector::ValidId(B.Flag),TEXT("needs flag, a story flag's id (a to z, 0 to 9 and _)"));break; // STORY-ON-FREEFORM-01 (step 4)
  case ES47Beat::Start:
   Need(B.Script.IsEmpty()!=B.Happening.IsEmpty(),TEXT("needs script (a story script) or happening (a night happening), not both"));
   Need(B.Script.IsEmpty()||(S47Script::CleanId(B.Script)==B.Script&&B.Script!=S.Id),TEXT("script is another story script's id"));
   break;
  case ES47Beat::Clock:Need(B.AtLeast>=8,TEXT("needs at_least, \"HH:MM\""));break;
  case ES47Beat::Tube:Need((B.Tube>=2&&B.Tube<=7&&B.Next==0)||(B.Tube<0&&B.Next>=1&&B.Next<=6),TEXT("needs tube, 2 to 7, or next, 1 to 6, not both (tubes 0 and 1 are the working light and are never put out)"));break; // N1110
  default:break;
  }
  // The story's text by its own check (it may carry the story's words, which Freeform's refuses); an objective may be empty (it clears).
  if(bStory){const TArray<FString> StoryWhy=B.Text.IsEmpty()?TArray<FString>():FS47StoryDirector::LintStory(B.Text);if(StoryWhy.Num())E.Add(At+TEXT(": text ")+StoryWhy[0]);}
  else{const FString Why=PlayerWords(B.Text);if(!Why.IsEmpty())E.Add(At+TEXT(": ")+Why);}
  for(const FString* T:{&B.Text,&B.Key,&B.Name})for(const FString& W:WordsIn(*T))if(!Words.Contains(W))Warn(FString::Printf(TEXT("%s: {%s} is not a word this event gives"),*At,*W));
  if(!B.Point.IsEmpty()&&!Points.Contains(B.Point))Warn(FString::Printf(TEXT("%s: point \"%s\" is not one this event gives"),*At,*B.Point));
 }
 return E;
}

TArray<FString> Lint(const FS47EventScript& S,TArray<FString>* Warnings){return LintAs(S,Warnings,false);}
TArray<FString> LintStory(const FS47EventScript& S,TArray<FString>* Warnings){return LintAs(S,Warnings,true);}

FString Fill(const FString& Text,const TMap<FString,FString>& Words)
{
 if(!Text.Contains(TEXT("{")))return Text;
 FString Out=Text;
 for(const auto& W:Words)Out.ReplaceInline(*(TEXT("{")+W.Key+TEXT("}")),*W.Value,ESearchCase::CaseSensitive);
 return Out;
}

bool GatesHold(const FS47EventGates& G,const FS47ScriptNow& Now,const FString& Area,double Roll,FString& Why,TFunction<bool(const FS47ScriptCond&)> Extra)
{
 using namespace S47EventScriptLocal;
 if(G.DayFrom>0&&Now.Day<G.DayFrom){Why=FString::Printf(TEXT("the script waits for day %d"),G.DayFrom);return false;}
 if(G.DayTo>0&&Now.Day>G.DayTo){Why=FString::Printf(TEXT("the script ended on day %d"),G.DayTo);return false;}
 if(G.HourFrom>=0&&G.HourTo>=0&&!S47Script::InHours(Now.Hour(),G.HourFrom,G.HourTo)){Why=TEXT("not the script's hours");return false;}
 if(G.bTier&&!S47NightTiers::Allows(G.Tier,Now.Hour(),Now.Night,G.bEarly)){Why=FString::Printf(TEXT("the night's tier is not %s yet"),S47NightTiers::Name(G.Tier));return false;}
 if(G.Areas.Num()&&!G.Areas.Contains(Area)){Why=TEXT("not in the script's areas");return false;}
 if(Roll>=G.Odds){Why=TEXT("the script's odds say not tonight");return false;}
 for(const FS47ScriptCond& C:G.If)
 {
  const bool bOk=OwnCond(C.If)?(Extra&&Extra(C)):S47Script::CondHolds(C,Now);
  if(!bOk){Why=FString::Printf(TEXT("the script's condition %s"),*C.If);return false;}
 }
 return true;
}

// N1110: eyes_window, beam_figure and dish_lights are the camera moments' (FS47Happenings::BeginMore), each with one named point.
// N1110-5: and the beats of four more happenings, each in the stages the code calls them at (horizon_figure, figure_near, landing, car_light).
const TArray<FString>& Wired()
{
 static const TArray<FString> L={TEXT("array_burst"),TEXT("town_blackout"),TEXT("hood_warm"),TEXT("eyes_window"),TEXT("beam_figure"),TEXT("dish_lights"),
  TEXT("horizon_figure"),TEXT("horizon_figure_gone"),TEXT("figure_near"),TEXT("figure_near_steps"),TEXT("figure_near_close"),TEXT("figure_near_light"),TEXT("figure_near_end"),
  TEXT("landing_warning"),TEXT("landing_seen_log"),TEXT("landing_logged"),TEXT("car_light"),TEXT("car_light_end"),
  TEXT("roof_steps"),TEXT("roof_steps_yard"),TEXT("roof_steps_end"),TEXT("s03_failure"),TEXT("s03_failure_mended")}; // N1110: the Director's two
 return L;
}
const TArray<FString>& WordsOf(const FString& Id)
{
 static const TArray<FString> Burst={TEXT("az"),TEXT("spread"),TEXT("target"),TEXT("burst_night")},Gone={TEXT("dir")},Ended={TEXT("place"),TEXT("seen"),TEXT("end")},
  Land={TEXT("az"),TEXT("spread"),TEXT("landing_night")},Lit={TEXT("outcome")},None;
 return Id==TEXT("array_burst")?Burst:Id==TEXT("horizon_figure_gone")?Gone:Id==TEXT("figure_near_end")?Ended:Id==TEXT("landing_seen_log")||Id==TEXT("landing_logged")?Land:Id==TEXT("car_light_end")?Lit:None;
}
const TArray<FString>& PointsOf(const FString& Id)
{
 static const TArray<FString> Truck={TEXT("truck")},Eyes={TEXT("eyes")},Figure={TEXT("figure")},Lights={TEXT("lights")},Roof={TEXT("roof")},None;
 return Id==TEXT("hood_warm")?Truck:Id==TEXT("eyes_window")?Eyes:Id==TEXT("beam_figure")||Id==TEXT("figure_near_steps")||Id==TEXT("figure_near_close")||Id==TEXT("figure_near_light")?Figure:Id==TEXT("dish_lights")?Lights:Id==TEXT("roof_steps_yard")?Roof:None;
}
const TArray<TPair<FString,FString>>& BuiltIn()
{
 static const TArray<TPair<FString,FString>> L=[]{TArray<TPair<FString,FString>> Out;for(const auto& P:S47EventScriptBuiltIn)Out.Add(TPair<FString,FString>(P[0],P[1]));return Out;}();
 return L;
}

TArray<FString> ReadDirs()
{
 TArray<FString> Dirs={FPaths::ProjectContentDir()/TEXT("Director/Events"),FPaths::ProjectSavedDir()/TEXT("Director/Events")};
 FString Own;
 if(FParse::Value(FCommandLine::Get(),TEXT("S47EventDir="),Own)&&!Own.IsEmpty())Dirs.Add(Own);
 TArray<FString> Out;
 for(FString D:Dirs){FPaths::NormalizeDirectoryName(D);FPaths::CollapseRelativeDirectories(D);Out.AddUnique(D);}
 return Out;
}

void Reload()
{
 using namespace S47EventScriptLocal;
 FLib& L=Lib();
 L.bRead=true;L.Bad.Reset();
 TMap<FString,FS47EventScript> ById;
 for(const auto& P:BuiltIn())
 {
  FS47EventScript S;FString Why;
  if(!Read(P.Value,S,Why)||S.Id!=P.Key){L.Bad.Add(TEXT("built in ")+P.Key,Why.IsEmpty()?TEXT("its id"):Why);continue;}
  S.Source=TEXT("built in");
  ById.Add(S.Id,S);
 }
 for(const FString& Dir:ReadDirs())
 {
  TArray<FString> Files;
  IFileManager::Get().FindFiles(Files,*(Dir/TEXT("*.json")),true,false);
  Files.Sort();
  for(const FString& File:Files)
  {
   const FString Path=Dir/File;
   FString Text,Why;FS47EventScript S;
   if(!FFileHelper::LoadFileToString(Text,*Path)){L.Bad.Add(Path,TEXT("could not be read"));continue;}
   if(!Read(Text,S,Why)){L.Bad.Add(Path,Why);UE_LOG(LogS47EventScript,Warning,TEXT("S47: event script %s refused: %s"),*Path,*Why);continue;}
   const TArray<FString> Errors=Lint(S);
   if(Errors.Num()){L.Bad.Add(Path,Errors[0]);UE_LOG(LogS47EventScript,Warning,TEXT("S47: event script %s will not play: %s"),*Path,*Errors[0]);continue;}
   // Switched off: an event with built-in beats keeps them.
   if(!S.bEnabled&&ById.Contains(S.Id)&&ById.FindChecked(S.Id).Source==TEXT("built in")){UE_LOG(LogS47EventScript,Display,TEXT("S47: event script %s is switched off: the built-in beats play"),*Path);continue;}
   S.Source=Path;
   ById.Add(S.Id,S);
  }
 }
 L.Scripts.Reset();
 for(auto& P:ById)L.Scripts.Add(P.Value);
 L.Scripts.Sort([](const FS47EventScript& A,const FS47EventScript& B){return A.Id<B.Id;});
 UE_LOG(LogS47EventScript,Display,TEXT("S47: event scripts: %d (%d from files), %d refused"),L.Scripts.Num(),L.Scripts.FilterByPredicate([](const FS47EventScript& S){return S.Source!=TEXT("built in");}).Num(),L.Bad.Num());
}

const TArray<FS47EventScript>& Library(){S47EventScriptLocal::ReadOnce();return S47EventScriptLocal::Lib().Scripts;}
const FS47EventScript* Find(const FString& Id){return Library().FindByPredicate([&Id](const FS47EventScript& S){return S.Id==Id;});}
const TMap<FString,FString>& Refused(){S47EventScriptLocal::ReadOnce();return S47EventScriptLocal::Lib().Bad;}

bool Allows(US47Main* M,const FString& Id,FString& Why)
{
 const FS47EventScript* S=Find(Id);
 if(!S||!S->Gates.Any())return true;
 FS47FreeformSession* F=M?M->Freeform.Get():nullptr;
 const FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 if(!F||!P)return true;
 const FS47ScriptNow Now=P->Now();
 const double Roll=FS47NightRhythm::Roll(F->Seed(),Now.Night,GetTypeHash(Id)^0x45565453u);
 const FString Area=M->Web?M->Web->Area():FString();
 return GatesHold(S->Gates,Now,Area,Roll,Why,[M,&Area](const FS47ScriptCond& C){return C.If==TEXT("in_saro")?FS47FieldRadio::InSaroBuilding(M):StoryOnlyCond(C.If)?FS47StoryDirector::CondHolds(M,C):C.If==TEXT("area")&&Area==C.Arg;});
}

bool Play(US47Main* M,const FString& Id,const FS47EventCtx& Ctx)
{
 const FS47EventScript* S=Find(Id);
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 FS47EventBeats* Runner=P?P->EventBeats():nullptr;
 if(!S||!Runner)
 {
  UE_LOG(LogS47EventScript,Display,TEXT("S47: event %s has %s"),*Id,!S?TEXT("no script"):TEXT("no runner here"));
  return false;
 }
 return Runner->Start(*S,Ctx);
}

bool CameraSnap(US47Main* M,const FS47CameraSnap& Snap,FString& Why)
{
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 FS47EventBeats* Runner=P?P->EventBeats():nullptr;
 if(!Runner){Why=TEXT("no runner here");return false;}
 return Runner->Snap(Snap,Why);
}
}

// ---------- the runner ----------

bool FS47EventBeats::Start(const FS47EventScript& S,const FS47EventCtx& Ctx){return Resume(S,Ctx,0,0);}

TArray<FS47EventBeats::FProgress> FS47EventBeats::Progress() const
{
 TArray<FProgress> Out;
 for(const FRun& R:Runs)Out.Add({R.S.Id,R.T,R.Next});
 return Out;
}

bool FS47EventBeats::Resume(const FS47EventScript& S,const FS47EventCtx& Ctx,double At,int32 Played)
{
 if(!M||Runs.Num()>=S47EventScriptLocal::MostRuns)return false;
 FRun R;R.S=S;R.Ctx=Ctx;
 // STORY-ON-FREEFORM-01 (step 4): taken up where a save left it (Start: from the beginning).
 R.T=FMath::Clamp(FMath::IsFinite(At)?At:0.,0.,S47EventScriptLocal::MostSeconds);R.Next=FMath::Clamp(Played,0,S.Beats.Num());
 // The words every event gives, unless its code gave its own.
 const FS47FreeformSession* F=M->Freeform.Get();
 const double Clock=F?F->Clock():0;
 const int32 Sec=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));
 if(R.Ctx.Words.FindRef(TEXT("clock")).IsEmpty())R.Ctx.Words.Add(TEXT("clock"),FString::Printf(TEXT("%02d:%02d"),Sec/3600,(Sec/60)%60));
 if(!R.Ctx.Words.Contains(TEXT("day")))R.Ctx.Words.Add(TEXT("day"),FString::FromInt(F?F->Day():1));
 if(!R.Ctx.Words.Contains(TEXT("night")))R.Ctx.Words.Add(TEXT("night"),FString::FromInt(FS47FreeformSession::NightOf(Clock)));
 if(!R.Ctx.Words.Contains(TEXT("map_key")))R.Ctx.Words.Add(TEXT("map_key"),S47Keys::MapSaid());
 UE_LOG(LogS47EventScript,Display,TEXT("S47: event script %s starts (%s, %d beats%s)"),*S.Id,*S.Source,S.Beats.Num(),R.Next>0||R.T>0?*FString::Printf(TEXT(", taken up at %.1f s after %d"),R.T,R.Next):TEXT(""));
 Runs.Add(R);
 // What comes at once comes now, before the event's code goes on.
 if(Advance(Runs.Last(),0))Runs.RemoveAt(Runs.Num()-1);
 return true;
}

void FS47EventBeats::Step(double Dt)
{
 if(LampLeft>0&&(LampLeft-=Dt)<=0)
 {
  LampLeft=0;
  if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,S47EventScriptLocal::LampFrom);
 }
 for(int32 I=Runs.Num()-1;I>=0;--I)if(Advance(Runs[I],Dt))Runs.RemoveAt(I);
}

void FS47EventBeats::Stop()
{
 Runs.Reset();
 if(Camera)Camera->Stop();
 if(LampLeft>0&&M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,S47EventScriptLocal::LampFrom);
 LampLeft=0;
}

bool FS47EventBeats::Advance(FRun& R,double Dt)
{
 R.T+=Dt;
 while(R.Next<R.S.Beats.Num())
 {
  const FS47Beat B=R.S.Beats[R.Next];
  if(B.At>R.T)return false;
  if(B.Wait.Num()&&!Holds(B.Wait))
  {
   // A beat that waits holds the ones after it; past its most it is left out.
   R.Waited+=Dt;
   if(R.Waited<B.Max)return false;
   UE_LOG(LogS47EventScript,Display,TEXT("S47: event script %s: beat %d (%s) waited %.0f s and is left out"),*R.S.Id,R.Next+1,S47EventScript::BeatName(B.Do),B.Max);
   ++R.Next;R.Waited=0;continue;
  }
  ++R.Next;R.Waited=0;
  if(B.If.IsEmpty()||Holds(B.If))Fire(B,R);
 }
 return true;
}

bool FS47EventBeats::Holds(const TArray<FS47ScriptCond>& Conds) const
{
 const FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 if(!P)return false;
 const FS47ScriptNow Now=P->Now();
 const FString Area=M->Web?M->Web->Area():FString();
 for(const FS47ScriptCond& C:Conds)
 {
  const bool bOk=C.If==TEXT("in_saro")?FS47FieldRadio::InSaroBuilding(M):C.If==TEXT("area")?Area==C.Arg
   :S47EventScript::StoryOnlyCond(C.If)?FS47StoryDirector::CondHolds(M,C):S47Script::CondHolds(C,Now); // STORY-ON-FREEFORM-01 (step 4)
  if(!bOk)return false;
 }
 return true;
}

bool FS47EventBeats::PosOf(const FS47Beat& B,const FRun& R,FVector& Out) const
{
 if(B.Pos.IsSet()){Out=B.Pos.GetValue();return true;}
 if(B.Point.IsEmpty())return false;
 if(const FVector* P=R.Ctx.Points.Find(B.Point)){Out=*P;return true;}
 if(B.Point==TEXT("here")&&M->Player){Out=FVector(M->Player->Pos.X,M->Player->FloorY+1,M->Player->Pos.Z);return true;}
 return false;
}

void FS47EventBeats::Fire(const FS47Beat& B,const FRun& R)
{
 using namespace S47EventScriptLocal;
 FS47FreeformSession* F=M->Freeform.Get();
 const FString Text=S47EventScript::Fill(B.Text,R.Ctx.Words),Name=S47EventScript::Fill(B.Name,R.Ctx.Words),Key=S47EventScript::Fill(B.Key,R.Ctx.Words);
 const auto Log=[&R,&B](const FString& What){UE_LOG(LogS47EventScript,Display,TEXT("S47: event script %s at %.1f s: %s %s"),*R.S.Id,B.At,S47EventScript::BeatName(B.Do),*What);};
 FVector At=FVector::ZeroVector;
 const bool bAt=PosOf(B,R,At);
 // A sign the gear, the lamp and the screens answer to (FS47Omen), at the point or the player.
 const auto Sign=[this,&R,&B,bAt,&At](const TCHAR* Tag)
 {
  if(!M->Omen)return false;
  FS47OmenSource Src;Src.Id=FString::Printf(TEXT("event:%s%s"),*R.S.Id,Tag);
  Src.At=bAt?At:(M->Player?FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z):FVector::ZeroVector);
  Src.Severity=FMath::Clamp(B.Strength,0.,1.);Src.Reach=FS47Omen::ReachFor(Src.Severity);
  M->Omen->Post(Src,FMath::Max(1.,B.Secs));
  return true;
 };
 switch(B.Do)
 {
 case ES47Beat::Sound:
 case ES47Beat::Printer:
 {
  if(!M->Audio){Log(TEXT("no sound"));break;}
  const bool bPrinter=B.Do==ES47Beat::Printer;
  FS47Sound O;O.Gain=float(FMath::Clamp(B.Gain,0.,2.));O.Rate=float(FMath::Clamp(B.Rate,.25,4.));O.Lp=float(FMath::Max(0.,B.Lp));O.When=float(FMath::Max(0.,B.Delay));
  if(bAt)O.At=At;else if(bPrinter)O.At=PrinterAt;
  if(!bPrinter&&B.Who==TEXT("radio"))O.Dest=TEXT("radio");
  else if(!bPrinter&&B.Who==TEXT("cb")){if(!FS47FieldRadio::CbAtHand(M)){Log(TEXT("not heard: no CB at hand"));break;}O.Dest=TEXT("radioBus");O.Lp=2600;}
  else if(!bPrinter&&B.Who==TEXT("walkie")){if(!M->FieldRadio||!M->FieldRadio->Owned()){Log(TEXT("not heard: no walkie"));break;}O.Lp=2200;O.Gain*=.7f;}
  const FS47Play Played=M->Audio->Sfx(bPrinter?FString(TEXT("printer")):Name,O);
  if(Played&&B.Stop>0&&M->Audio->Ctx.IsValid())Played.Src->Stop(M->Audio->Ctx->CurrentTime()+B.Stop);
  if(bPrinter&&!Text.IsEmpty()&&M->UI)M->UI->Toast(Text,float(B.Secs>0?B.Secs:4));
  Log(bPrinter?FString(TEXT("the rack")):Name);
  break;
 }
 case ES47Beat::Tone:
  if(!M->Audio)break;
  for(int32 K=0;K<FMath::Clamp(B.Repeat,1,32);++K)M->Audio->Beep(float(B.Hz),float(B.Secs>0?B.Secs:.08),float(B.Gain),float(FMath::Max(0.,B.Delay)+K*FMath::Max(0.,B.Every)),B.Kind);
  Log(FString::Printf(TEXT("%.0f Hz x%d"),B.Hz,B.Repeat));
  break;
 case ES47Beat::Light:
  if(B.Kind==TEXT("town"))
  {
   FS47Town* T=M->TownLayer?M->TownLayer->Town.Get():nullptr;
   if(T)T->LampWave(M->T);
   Log(T?TEXT("Caliche's lamps"):TEXT("no town here"));
  }
  else if(B.Kind==TEXT("lamp"))
  {
   FS47Flashlight* L=M->Night?M->Night->Flashlight.Get():nullptr;
   if(L){L->Flicker(FMath::Clamp(B.Strength,0.,1.),LampFrom);LampLeft=FMath::Max(LampLeft,B.Secs>0?B.Secs:1.5);}
   Log(L?TEXT("the flashlight"):TEXT("no flashlight"));
  }
  else Log(Sign(TEXT(""))?TEXT("the gear"):TEXT("no signs here"));
  break;
 case ES47Beat::Tv:
  if(M->Audio)M->Audio->TvHiss(bAt?TOptional<FVector>(At):TOptional<FVector>());
  if(B.Secs>0&&B.Strength>0)Sign(TEXT(":tv"));
  Log(TEXT(""));
  break;
 case ES47Beat::Line:
 {
  bool bSaid=false;
  if(B.Who==TEXT("dale"))bSaid=M->FieldRadio&&M->FieldRadio->Message(FString(TEXT("DALE: "))+Text);
  else if(B.Who==TEXT("voice"))bSaid=M->FieldRadio&&M->FieldRadio->Message(Text);
  else if(B.Who==TEXT("cb"))bSaid=FS47FieldRadio::CbSay(M,{Text},TEXT("man"));
  else if(B.Who==TEXT("radio")&&M->Radio&&F)
  {
   const TSharedPtr<FS47Station>* St=M->Radio->Stations.Find(TEXT("caliche"));
   bSaid=St&&St->IsValid()&&(*St)->CutIn({FS47RadioLine(Text,TEXT("host"),TEXT("the host"))},FMath::Fmod(F->Clock(),86400.))>0;
  }
  Log(bSaid?B.Who:B.Who+TEXT(": not heard (no set, or busy)"));
  break;
 }
 case ES47Beat::Thought:
  if(M->UI)M->UI->Thought(Text,float(B.Secs>0?B.Secs:4.5));
  Log(TEXT(""));
  break;
 case ES47Beat::Toast:
  if(M->UI)M->UI->Toast(Text,float(B.Secs>0?B.Secs:4));
  Log(TEXT(""));
  break;
 case ES47Beat::Evidence:
  if(!F)break;
  if(R.Ctx.bKeep){F->RecordEvidence(Key,Text);Log(Key);}
  else{if(M->UI)M->UI->Toast(FString::Printf(TEXT("(F10: the finding %s is not kept in a real game)"),*Key),3);Log(TEXT("F10: not kept"));}
  break;
 case ES47Beat::Map:
  if(F&&F->Places&&FS47Places::Def(Name)){F->Places->Hear(Name);Log(Name);}
  else Log(Name.IsEmpty()?FString(TEXT("no place")):Name+TEXT(": not a place on the map"));
  break;
 case ES47Beat::Prop:
 case ES47Beat::Show:
 case ES47Beat::Hide:
 {
  const int32 Node=M->Web?M->Web->Handle(Name):-1;
  if(Node<0){Log(Name+TEXT(": not in this scene"));break;}
  if(B.Do==ES47Beat::Prop){const FS47WebNode* N=M->Web->Node(Node);if(N)M->Web->SetPosition(Node,B.bTo?B.By:N->P+B.By);}
  else M->Web->SetVisible(Node,B.Do==ES47Beat::Show);
  Log(Name);
  break;
 }
 case ES47Beat::Shake:
  if(M->Player)M->Player->Shake=FMath::Max(M->Player->Shake,FMath::Clamp(B.Strength,0.,1.));
  Log(TEXT(""));
  break;
 case ES47Beat::CameraSnap:
 {
  // N1110: the sting first (it is the sight that hits: whatever was heard last, as F10's audition), then the view toward the point.
  FString What;
  if(R.Ctx.bNoSnap){Log(TEXT("left out: not the hour for it yet"));break;}
  if(!B.Kind.IsEmpty()&&M->FieldRadio)
  {
   const int32 Scare=B.Kind==TEXT("soft")?int32(FS47EncounterSound::EScare::Soft):B.Kind==TEXT("hard")?int32(FS47EncounterSound::EScare::Hard):B.Kind==TEXT("braam")?int32(FS47EncounterSound::EScare::Braam):int32(FS47EncounterSound::EScare::Swell);
   What=M->FieldRadio->Scare(Scare,1.,true)?B.Kind+TEXT(" hit, "):B.Kind+TEXT(" hit missing (no file), ");
  }
  if(!bAt){Log(What+TEXT("no point to look at"));break;}
  FS47CameraSnap S;S.At=At;S.Fov=B.Fov;S.In=B.In;S.Hold=B.Hold;S.Out=B.Out;S.Shake=FMath::Clamp(B.Strength,0.,1.);
  FString Why;
  Log(What+(Snap(S,Why)?FString::Printf(TEXT("the view to %.0f degrees for %.1f s"),B.Fov,B.In+B.Hold+B.Out):TEXT("the view stays: ")+Why));
  break;
 }
 case ES47Beat::Silence:
 {
  // N1110 (SOUNDSCAPE): the night's own bed (crickets, wind, the far-off ones) drops at once and comes back slowly after secs.
  FS47Ambience* Bed=M->Audio?M->Audio->Night.Get():nullptr;
  if(Bed)Bed->Hush(FMath::Clamp(B.Secs,.5,30.),FMath::Clamp(B.Strength,0.,1.),.12,3.5);
  Log(Bed?FString::Printf(TEXT("the night holds still for %.1f s"),B.Secs):FString(TEXT("no night bed here")));
  break;
 }
 case ES47Beat::Tube:
 {
  // N1110 (PLAY-31-34): a control room ceiling tube sparks and goes out for good. The light follows the saved break (FS47FreeformSession::
  // TubeBroken, read by AS47Web::TubeLit); an F10 run keeps nothing, so the tube only crackles.
  // {"beat":"tube","next":2}: the next still-working tubes of 2 to 7, lowest first, so each sabotage darkens the room a little more.
  TArray<int32> Hit;
  if(F&&B.Tube<0&&B.Next>=1&&B.Next<=6)Hit=F->NextTubes(B.Next);
  else if(F&&B.Tube>=2&&B.Tube<=7&&!F->TubeBroken(B.Tube))Hit.Add(B.Tube);
  else if(!F||B.Tube<2||B.Tube>7){Log(TEXT("no such tube"));break;}
  for(const int32 K:Hit)
  {
   if(R.Ctx.bKeep)F->BreakTube(K);
   if(M->Audio&&FS47FieldRadio::InSaroBuilding(M))
    for(int32 J=0;J<10;++J)M->Audio->Beep(float(900+((J*7919+K*131)%1700)),float(.02+.01*(J%3)),.05f,float(J*.045+(J%2)*.03),TEXT("sawtooth"));
   if(!R.Ctx.bKeep&&M->UI)M->UI->Toast(FString::Printf(TEXT("(F10: tube %d crackles but is not put out in a real game)"),K),3);
   Log(FString::Printf(TEXT("tube %d sparks and is out"),K));
  }
  if(Hit.IsEmpty())Log(B.Next>0?TEXT("no working tube is left to put out"):FString::Printf(TEXT("tube %d was already out"),B.Tube));
  break;
 }
 case ES47Beat::Flag:
 case ES47Beat::Start:
 case ES47Beat::Clock:
 case ES47Beat::Objective:
 {
  // STORY-ON-FREEFORM-01 (step 4): the story's own beats are the story director's; without a Story night they do nothing.
  FS47StoryDirector* Story=FS47StoryDirector::Of(M);
  Log(Story?Story->Apply(B,Text):FString(TEXT("left out: no story director")));
  break;
 }
 default:break;
 }
}

bool FS47EventBeats::Snap(const FS47CameraSnap& S,FString& Why)
{
 if(!M){Why=TEXT("no game");return false;}
 if(!Camera)Camera=MakeShared<FS47CameraMoment>(M);
 return Camera->Start(S,Why);
}

bool FS47EventBeats::DevRun(const FString& Id,FString& Why)
{
 const FS47EventScript* S=S47EventScript::Find(Id);
 if(!S){Why=TEXT("no script ")+Id;return false;}
 if(!M||!M->Freeform||!M->Freeform->Active()){Why=TEXT("not in Freeform");return false;}
 FS47EventCtx Ctx;
 // A real game keeps nothing of an F10 run; a developer's run keeps what it records.
 Ctx.bKeep=M->Flow&&M->Flow->bDevRun;
 // Sample words where the event's code would give its own.
 const FS47FreeformSession* F=M->Freeform.Get();
 Ctx.Words.Add(TEXT("az"),TEXT("120"));Ctx.Words.Add(TEXT("spread"),TEXT("18"));Ctx.Words.Add(TEXT("target"),FString());
 Ctx.Words.Add(TEXT("burst_night"),FString::FromInt(F->Day()));
 // (N1110-5: and for the stages of horizon_figure, figure_near, landing and car_light)
 Ctx.Words.Add(TEXT("dir"),TEXT("north-east"));Ctx.Words.Add(TEXT("place"),TEXT("the open ground"));Ctx.Words.Add(TEXT("seen"),TEXT("a figure stood off to one side, about 60 metres off, dead still."));
 Ctx.Words.Add(TEXT("end"),TEXT(" I walked away fast and kept walking."));Ctx.Words.Add(TEXT("landing_night"),FString::FromInt(F->Day()));Ctx.Words.Add(TEXT("outcome"),TEXT(" I stayed in the cab, and after about twenty seconds it went up."));
 // (N1110: a point a camera moment looks at is put six metres ahead, so that the snap is seen to turn)
 if(M->Player)for(const FString& P:S47EventScript::PointsOf(Id))
 {
  const bool bAhead=P==TEXT("eyes")||P==TEXT("figure")||P==TEXT("lights")||P==TEXT("roof");
  const FVector Ahead=bAhead?FVector(-FMath::Sin(M->Player->Yaw)*6,P==TEXT("lights")?4.:.6,-FMath::Cos(M->Player->Yaw)*6):FVector::ZeroVector;
  Ctx.Points.Add(P,FVector(M->Player->Pos.X,M->Player->FloorY+1,M->Player->Pos.Z)+Ahead);
 }
 if(!Start(*S,Ctx)){Why=TEXT("too many scripts playing");return false;}
 Why=FString::Printf(TEXT("%s, %d beats (%s)"),*Id,S->Beats.Num(),S->Source==TEXT("built in")?TEXT("built in"):TEXT("from its file"));
 return true;
}

void FS47EventBeats::AddDevLines()
{
 if(!M||!M->DevEvents)return;
 static const TCHAR* const Group=TEXT("Event scripts");
 US47Main* Main=M;
 const auto Have=[Main](const FString& Label){return Main->DevEvents->Events().ContainsByPredicate([&Label](const FS47DevEvents::FEvent& E){return E.Group==Group&&E.Label==Label;});};
 // The menu outlives a session: each line is added once.
 const FString Again=TEXT("Read the event scripts again");
 if(!LinesFor.Contains(Again))
 {
  LinesFor.Add(Again);
  if(!Have(Again))M->DevEvents->AddFreeform(Group,Again,TEXT("Content/Director/Events, Saved/Director/Events; a new file gets its own line"),[Main]
  {
   S47EventScript::Reload();
   if(Main->UI)Main->UI->Toast(FString::Printf(TEXT("Developer event: %d event scripts, %d refused%s."),S47EventScript::Library().Num(),S47EventScript::Refused().Num(),S47EventScript::Refused().Num()?*(TEXT(" (first: ")+S47EventScript::Refused().CreateConstIterator().Value()+TEXT(")")):TEXT("")),5);
  });
 }
 for(const FS47EventScript& S:S47EventScript::Library())
 {
  const FString Label=TEXT("Run: ")+S.Id;
  if(LinesFor.Contains(Label))continue;
  LinesFor.Add(Label);
  if(Have(Label))continue;
  const FString Id=S.Id;
  M->DevEvents->AddFreeform(Group,Label,S.Name.IsEmpty()?FString(TEXT("its beats now, with sample words")):S.Name+TEXT("; its beats now, with sample words"),[Main,Id]
  {
   FS47ScriptPlayer* P=FS47ScriptPlayer::Of(Main);
   FS47EventBeats* Runner=P?P->EventBeats():nullptr;
   FString Why=TEXT("no runner here");
   const bool bOk=Runner&&Runner->DevRun(Id,Why);
   if(Main->UI)Main->UI->Toast(FString(TEXT("Developer event: "))+(bOk?TEXT("event script "):TEXT("not now: "))+Why,4);
  });
 }
}
