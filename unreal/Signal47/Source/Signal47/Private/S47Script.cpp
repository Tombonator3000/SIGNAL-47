#include "S47Script.h"

// DIRECTOR-TOOL-20261109: the scripted events' data, rules and lint (S47Script.h). The player is S47ScriptPlayer.cpp, the editor
// SS47ScriptEditor.cpp, the tests S47ScriptTest.cpp.

namespace S47ScriptLocal
{
 const TCHAR* const ActNames[int32(ES47ScriptAct::Count)]={TEXT("figure"),TEXT("sky"),TEXT("sound"),TEXT("dish"),TEXT("fault"),TEXT("line"),TEXT("toast"),TEXT("note"),TEXT("evidence"),TEXT("look"),TEXT("wait"),TEXT("shot")};
 S47J::FObj PointJson(const FS47ScriptPoint& P)
 {
  const S47J::FObj O=S47J::New();
  S47J::Set(O,TEXT("x"),FMath::RoundToDouble(P.X*10)/10);S47J::Set(O,TEXT("z"),FMath::RoundToDouble(P.Z*10)/10);
  if(P.Up!=0)S47J::Set(O,TEXT("up"),P.Up);
  return O;
 }
 FS47ScriptPoint PointFrom(const S47J::FObj& O)
 {
  FS47ScriptPoint P;
  if(!O.IsValid()||!S47J::Has(O,TEXT("x"))||!S47J::Has(O,TEXT("z")))return P;
  P.X=S47J::Num(O,TEXT("x"));P.Z=S47J::Num(O,TEXT("z"));P.Up=S47J::Num(O,TEXT("up"));
  P.bSet=FMath::IsFinite(P.X)&&FMath::IsFinite(P.Z);
  return P;
 }
 void Range(const S47J::FObj& O,const TCHAR* K,int32 From,int32 To)
 {
  if(From<=0&&To<=0)return;
  O->SetArrayField(K,{MakeShared<FJsonValueNumber>(From),MakeShared<FJsonValueNumber>(To)});
 }
 void RangeFrom(const S47J::FObj& O,const TCHAR* K,int32& From,int32& To)
 {
  const TArray<double> V=S47J::Nums(O,K);
  From=V.Num()>0?FMath::Max(0,int32(V[0])):0;To=V.Num()>1?FMath::Max(0,int32(V[1])):From;
 }
 bool In(const TArray<FString>& List,const FString& What){return List.Contains(What);}

 // ---------- the lint's words ----------
 // A word: letters (with the accented ones), digits, and a point or comma between two digits ("8.15").
 struct FWord{FString Low,Raw;int32 Start=0;};
 bool WordChar(TCHAR C){return FChar::IsAlnum(C)||C>=0xC0;}
 TArray<FWord> Words(const FString& Text)
 {
  TArray<FWord> Out;
  const int32 N=Text.Len();
  for(int32 I=0;I<N;)
  {
   if(!WordChar(Text[I])){++I;continue;}
   const int32 S=I;
   while(I<N&&(WordChar(Text[I])||((Text[I]=='.'||Text[I]==',')&&I>S&&I+1<N&&FChar::IsDigit(Text[I-1])&&FChar::IsDigit(Text[I+1]))))++I;
   FWord W;W.Raw=Text.Mid(S,I-S);W.Low=W.Raw.ToLower();W.Start=S;
   // Tomás, Tomas: the accent goes for the comparison.
   W.Low.ReplaceInline(TEXT("á"),TEXT("a"));W.Low.ReplaceInline(TEXT("Á"),TEXT("a"));
   Out.Add(W);
  }
  return Out;
 }
 bool AnyOf(const FString& W,std::initializer_list<const TCHAR*> List){for(const TCHAR* L:List)if(W==L)return true;return false;}
 bool StartsAny(const FString& W,std::initializer_list<const TCHAR*> List){for(const TCHAR* L:List)if(W.StartsWith(L))return true;return false;}
 // Within Span words after I (or before, Span negative) one of these.
 bool Near(const TArray<FWord>& W,int32 I,int32 Span,std::initializer_list<const TCHAR*> List,bool bPrefix=false)
 {
  const int32 A=Span>0?I+1:FMath::Max(0,I+Span),B=Span>0?FMath::Min(W.Num()-1,I+Span):I-1;
  for(int32 K=A;K<=B;++K)if(bPrefix?StartsAny(W[K].Low,List):AnyOf(W[K].Low,List))return true;
  return false;
 }
}

bool FS47Script::HasFigure() const
{
 return Actions.ContainsByPredicate([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure;});
}

bool FS47Script::HasSky() const
{
 return Actions.ContainsByPredicate([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Sky;});
}

ES47Allowed FS47Script::Need() const
{
 const ES47Allowed Own=HasFigure()||bTierSet?Tier:ES47Allowed::Hint;
 return HasSky()&&uint8(Own)<uint8(ES47Allowed::Glimpse)?ES47Allowed::Glimpse:Own;
}

double FS47Script::Length() const
{
 double End=0,Waits=0;
 for(const FS47ScriptAction& A:Actions)
 {
  double Hold=0;
  switch(A.Do)
  {
  case ES47ScriptAct::Figure:{double Way=0;for(int32 I=1;I<A.Path.Num();++I)Way+=FVector2D::Distance(A.Path[I-1].Road(),A.Path[I].Road());Hold=Way/FMath::Max(.2,A.Speed)+A.Secs;break;}
  case ES47ScriptAct::Sky:Hold=A.Who==TEXT("fast")?24:95;break;
  case ES47ScriptAct::Wait:Waits+=A.Secs;break;
  case ES47ScriptAct::Fault:case ES47ScriptAct::Dish:case ES47ScriptAct::Look:case ES47ScriptAct::Toast:Hold=A.Secs;break;
  case ES47ScriptAct::Shot:Hold=A.In+A.Secs+A.Out;break; // SHOT-BEAT-01: in, held, back
  default:break;
  }
  End=FMath::Max(End,A.At+Hold);
 }
 return End+Waits;
}

S47J::FObj FS47ScriptMemory::Save() const
{
 const S47J::FObj O=S47J::New();
 for(const auto& P:Count)
 {
  const S47J::FObj E=S47J::New();
  S47J::Set(E,TEXT("count"),P.Value);S47J::Set(E,TEXT("night"),LastNight.FindRef(P.Key));S47J::Set(E,TEXT("clock"),LastClock.FindRef(P.Key));
  O->SetObjectField(P.Key,E);
 }
 return O;
}

void FS47ScriptMemory::Load(const S47J::FObj& O)
{
 Count.Reset();LastNight.Reset();LastClock.Reset();
 if(!O.IsValid())return;
 for(const auto& P:O->Values)
 {
  const S47J::FObj* E=nullptr;
  if(!P.Value.IsValid()||!P.Value->TryGetObject(E)||!E)continue;
  const FString Id(*P.Key);
  Count.Add(Id,S47J::Int(*E,TEXT("count")));LastNight.Add(Id,S47J::Int(*E,TEXT("night")));LastClock.Add(Id,S47J::Num(*E,TEXT("clock")));
 }
}

namespace S47Script
{
const TCHAR* ActName(ES47ScriptAct A){return int32(A)<int32(ES47ScriptAct::Count)?S47ScriptLocal::ActNames[int32(A)]:TEXT("?");}
ES47ScriptAct ActFrom(const FString& Name)
{
 for(int32 I=0;I<int32(ES47ScriptAct::Count);++I)if(Name==S47ScriptLocal::ActNames[I])return ES47ScriptAct(I);
 return ES47ScriptAct::Count;
}
const TCHAR* RepeatName(ES47ScriptRepeat R){return R==ES47ScriptRepeat::Nightly?TEXT("nightly"):R==ES47ScriptRepeat::Always?TEXT("always"):TEXT("once");}
ES47ScriptRepeat RepeatFrom(const FString& Name){return Name==TEXT("nightly")?ES47ScriptRepeat::Nightly:Name==TEXT("always")?ES47ScriptRepeat::Always:ES47ScriptRepeat::Once;}
const TArray<FString>& Bodies(){static const TArray<FString> L={TEXT("pale"),TEXT("tall"),TEXT("crab")};return L;}
const TArray<FString>& Forms(){static const TArray<FString> L={TEXT("v"),TEXT("line"),TEXT("triangle"),TEXT("circle"),TEXT("scatter"),TEXT("high_track")};return L;}
const TArray<FString>& Paces(){static const TArray<FString> L={TEXT("slow"),TEXT("fast"),TEXT("out")};return L;}
const TArray<FString>& SoundWhere(){static const TArray<FString> L={TEXT("here"),TEXT("placed"),TEXT("radio"),TEXT("cb"),TEXT("walkie")};return L;}
const TArray<FString>& FaultKinds(){static const TArray<FString> L={TEXT("omen"),TEXT("contact"),TEXT("power")};return L;}
const TArray<FString>& Voices(){static const TArray<FString> L={TEXT("dale"),TEXT("radio"),TEXT("cb"),TEXT("voice")};return L;}
const TArray<FString>& CondKinds(){static const TArray<FString> L={TEXT("finding"),TEXT("no_finding"),TEXT("case_open"),TEXT("case_closed"),TEXT("case_none"),TEXT("weather"),TEXT("moon_min"),TEXT("moon_max"),TEXT("moon_up"),TEXT("indoors"),TEXT("outdoors"),TEXT("in_truck"),TEXT("on_foot"),TEXT("near"),TEXT("far")};return L;}
const TArray<FString>& WeatherWords(){static const TArray<FString> L={TEXT("clear"),TEXT("cloudy"),TEXT("windy"),TEXT("calm"),TEXT("dust"),TEXT("shower"),TEXT("storm")};return L;}
const TArray<FString>& SoundPresets(){static const TArray<FString> L={TEXT("thudSoft"),TEXT("doorCreak0"),TEXT("doorCreak1"),TEXT("doorMetal"),TEXT("gust"),TEXT("phoneRing"),TEXT("switch"),TEXT("engineIdle"),TEXT("printer")};return L;}

S47J::FObj ToJson(const FS47Script& S)
{
 using namespace S47ScriptLocal;
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("version"),FS47Script::Version);
 S47J::Set(O,TEXT("id"),S.Id);S47J::Set(O,TEXT("name"),S.Name);S47J::Set(O,TEXT("enabled"),S.bEnabled);
 S47J::Set(O,TEXT("repeat"),RepeatName(S.Repeat));
 if(S.Repeat==ES47ScriptRepeat::Always)S47J::Set(O,TEXT("cooldown_min"),S.CooldownMin);
 if(S.Tier!=ES47Allowed::Full||S.bTierSet)S47J::Set(O,TEXT("tier"),S47NightTiers::Name(S.Tier));
 if(S.bEarly)S47J::Set(O,TEXT("early"),true);
 const S47J::FObj T=S47J::New();
 const FS47ScriptTrigger& Tr=S.Trigger;
 Range(T,TEXT("days"),Tr.DayFrom,Tr.DayTo);Range(T,TEXT("nights"),Tr.NightFrom,Tr.NightTo);
 if(Tr.HourFrom>=0&&Tr.HourTo>=0)S47J::Set(T,TEXT("hours"),TArray<double>{Tr.HourFrom,Tr.HourTo});
 if(!Tr.Place.IsEmpty())S47J::Set(T,TEXT("place"),Tr.Place);
 else if(Tr.Point.bSet)S47J::Set(T,TEXT("point"),PointJson(Tr.Point));
 if(Tr.HasWhere())S47J::Set(T,TEXT("radius"),Tr.Radius);
 TArray<TSharedPtr<FJsonValue>> Cs;
 for(const FS47ScriptCond& C:Tr.Conds)
 {
  const S47J::FObj J=S47J::New();S47J::Set(J,TEXT("if"),C.If);
  if(C.If==TEXT("finding")||C.If==TEXT("no_finding"))S47J::Set(J,TEXT("id"),C.Arg);
  else if(C.If.StartsWith(TEXT("case_")))S47J::Set(J,TEXT("case"),int32(C.Num));
  else if(C.If==TEXT("weather"))S47J::Set(J,TEXT("is"),C.Arg);
  else if(C.If==TEXT("moon_min")||C.If==TEXT("moon_max"))S47J::Set(J,TEXT("lit"),C.Num);
  else if(C.If==TEXT("near")||C.If==TEXT("far")){S47J::Set(J,TEXT("place"),C.Arg);S47J::Set(J,TEXT("m"),C.Num);}
  Cs.Add(MakeShared<FJsonValueObject>(J));
 }
 if(Cs.Num())T->SetArrayField(TEXT("conditions"),Cs);
 S47J::Set(O,TEXT("trigger"),T);
 TArray<TSharedPtr<FJsonValue>> As;
 for(const FS47ScriptAction& A:S.Actions)
 {
  const S47J::FObj J=S47J::New();
  S47J::Set(J,TEXT("at"),A.At);S47J::Set(J,TEXT("do"),ActName(A.Do));
  switch(A.Do)
  {
  case ES47ScriptAct::Figure:
  {
   S47J::Set(J,TEXT("body"),A.Kind);S47J::Set(J,TEXT("speed"),A.Speed);S47J::Set(J,TEXT("stand"),A.Secs);
   if(A.bFlag)S47J::Set(J,TEXT("vanish_seen"),true);
   TArray<TSharedPtr<FJsonValue>> P;for(const FS47ScriptPoint& Pt:A.Path)P.Add(MakeShared<FJsonValueObject>(PointJson(Pt)));
   J->SetArrayField(TEXT("path"),P);break;
  }
  case ES47ScriptAct::Sky:S47J::Set(J,TEXT("form"),A.Kind);S47J::Set(J,TEXT("pace"),A.Who);break;
  case ES47ScriptAct::Sound:S47J::Set(J,TEXT("name"),A.Text);S47J::Set(J,TEXT("where"),A.Who);S47J::Set(J,TEXT("gain"),A.Gain);if(A.Point.bSet)S47J::Set(J,TEXT("point"),PointJson(A.Point));break;
  case ES47ScriptAct::Dish:S47J::Set(J,TEXT("az"),A.Az);S47J::Set(J,TEXT("el"),A.El);S47J::Set(J,TEXT("slew"),A.Slew);S47J::Set(J,TEXT("hold"),A.Secs);if(A.Index>=0)S47J::Set(J,TEXT("dish"),A.Index);break;
  case ES47ScriptAct::Fault:S47J::Set(J,TEXT("kind"),A.Kind);S47J::Set(J,TEXT("severity"),A.Gain);S47J::Set(J,TEXT("secs"),A.Secs);if(A.Point.bSet)S47J::Set(J,TEXT("point"),PointJson(A.Point));break;
  case ES47ScriptAct::Line:S47J::Set(J,TEXT("who"),A.Who);S47J::Set(J,TEXT("text"),A.Text);break;
  case ES47ScriptAct::Toast:S47J::Set(J,TEXT("text"),A.Text);S47J::Set(J,TEXT("secs"),A.Secs);break;
  case ES47ScriptAct::Note:S47J::Set(J,TEXT("title"),A.Title);S47J::Set(J,TEXT("text"),A.Text);break;
  case ES47ScriptAct::Evidence:S47J::Set(J,TEXT("id"),A.Kind);S47J::Set(J,TEXT("text"),A.Text);break;
  case ES47ScriptAct::Look:S47J::Set(J,TEXT("secs"),A.Secs);if(A.Point.bSet)S47J::Set(J,TEXT("point"),PointJson(A.Point));break;
  case ES47ScriptAct::Wait:S47J::Set(J,TEXT("secs"),A.Secs);if(A.bFlag){S47J::Set(J,TEXT("until_look"),true);if(A.Point.bSet)S47J::Set(J,TEXT("point"),PointJson(A.Point));}break;
  case ES47ScriptAct::Shot: // SHOT-BEAT-01
   if(A.Path.Num()>0)S47J::Set(J,TEXT("eye"),PointJson(A.Path[0]));
   if(A.Path.Num()>1)S47J::Set(J,TEXT("eye2"),PointJson(A.Path[1]));
   if(A.Point.bSet)S47J::Set(J,TEXT("point"),PointJson(A.Point));
   S47J::Set(J,TEXT("fov"),A.Fov);if(A.Fov2>0)S47J::Set(J,TEXT("fov2"),A.Fov2);
   S47J::Set(J,TEXT("secs"),A.Secs);S47J::Set(J,TEXT("in"),A.In);S47J::Set(J,TEXT("out"),A.Out);
   break;
  default:break;
  }
  As.Add(MakeShared<FJsonValueObject>(J));
 }
 O->SetArrayField(TEXT("actions"),As);
 return O;
}

bool FromJson(const S47J::FObj& O,FS47Script& Out,FString& Why)
{
 using namespace S47ScriptLocal;
 Out=FS47Script();
 if(!O.IsValid()){Why=TEXT("does not parse");return false;}
 if(S47J::Int(O,TEXT("version"))!=FS47Script::Version){Why=TEXT("version is not 1");return false;}
 Out.Id=S47J::Str(O,TEXT("id"));Out.Name=S47J::Str(O,TEXT("name"),Out.Id);Out.bEnabled=S47J::Bool(O,TEXT("enabled"));
 if(Out.Id.IsEmpty()){Why=TEXT("no id");return false;}
 Out.Repeat=RepeatFrom(S47J::Str(O,TEXT("repeat"),TEXT("once")));Out.CooldownMin=FMath::Max(0.,S47J::Num(O,TEXT("cooldown_min"),60));
 {const FString TierName=S47J::Str(O,TEXT("tier"),TEXT("full"));if(!S47NightTiers::FromName(TierName,Out.Tier)){Why=TEXT("tier is hint, glimpse or full");return false;}}
 Out.bTierSet=O->HasField(TEXT("tier"));
 Out.bEarly=S47J::Bool(O,TEXT("early"));
 const S47J::FObj T=S47J::Obj(O,TEXT("trigger"));
 FS47ScriptTrigger& Tr=Out.Trigger;
 RangeFrom(T,TEXT("days"),Tr.DayFrom,Tr.DayTo);RangeFrom(T,TEXT("nights"),Tr.NightFrom,Tr.NightTo);
 const TArray<double> H=S47J::Nums(T,TEXT("hours"));
 if(H.Num()==2&&FMath::IsFinite(H[0])&&FMath::IsFinite(H[1])){Tr.HourFrom=FMath::Fmod(FMath::Max(0.,H[0]),24.);Tr.HourTo=FMath::Fmod(FMath::Max(0.,H[1]),24.);}
 Tr.Place=S47J::Str(T,TEXT("place"));Tr.Point=PointFrom(S47J::Obj(T,TEXT("point")));
 Tr.Radius=FMath::Max(1.,S47J::Num(T,TEXT("radius"),40));
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(T,TEXT("conditions")))
 {
  const S47J::FObj* J=nullptr;if(!V.IsValid()||!V->TryGetObject(J)||!J)continue;
  FS47ScriptCond C;C.If=S47J::Str(*J,TEXT("if"));
  if(C.If==TEXT("finding")||C.If==TEXT("no_finding"))C.Arg=S47J::Str(*J,TEXT("id"));
  else if(C.If.StartsWith(TEXT("case_")))C.Num=S47J::Num(*J,TEXT("case"),-1);
  else if(C.If==TEXT("weather"))C.Arg=S47J::Str(*J,TEXT("is"));
  else if(C.If==TEXT("moon_min")||C.If==TEXT("moon_max"))C.Num=S47J::Num(*J,TEXT("lit"));
  else if(C.If==TEXT("near")||C.If==TEXT("far")){C.Arg=S47J::Str(*J,TEXT("place"));C.Num=S47J::Num(*J,TEXT("m"),100);}
  Tr.Conds.Add(C);
 }
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(O,TEXT("actions")))
 {
  const S47J::FObj* JP=nullptr;if(!V.IsValid()||!V->TryGetObject(JP)||!JP)continue;
  const S47J::FObj& J=*JP;
  FS47ScriptAction A;A.Do=ActFrom(S47J::Str(J,TEXT("do")));
  if(A.Do==ES47ScriptAct::Count){Why=FString::Printf(TEXT("an action does \"%s\", which is not known"),*S47J::Str(J,TEXT("do")));return false;}
  A.At=FMath::Max(0.,S47J::Num(J,TEXT("at")));
  A.Point=PointFrom(S47J::Obj(J,TEXT("point")));
  switch(A.Do)
  {
  case ES47ScriptAct::Figure:
   A.Kind=S47J::Str(J,TEXT("body"),TEXT("pale"));A.Speed=S47J::Num(J,TEXT("speed"),1);A.Secs=S47J::Num(J,TEXT("stand"),6);A.bFlag=S47J::Bool(J,TEXT("vanish_seen"));
   for(const TSharedPtr<FJsonValue>& P:S47J::Arr(J,TEXT("path"))){const S47J::FObj* PO=nullptr;if(P.IsValid()&&P->TryGetObject(PO)&&PO){const FS47ScriptPoint Pt=PointFrom(*PO);if(Pt.bSet)A.Path.Add(Pt);}}
   break;
  case ES47ScriptAct::Sky:A.Kind=S47J::Str(J,TEXT("form"),TEXT("v"));A.Who=S47J::Str(J,TEXT("pace"),TEXT("slow"));break;
  case ES47ScriptAct::Sound:A.Text=S47J::Str(J,TEXT("name"));A.Who=S47J::Str(J,TEXT("where"),TEXT("here"));A.Gain=S47J::Num(J,TEXT("gain"),1);break;
  case ES47ScriptAct::Dish:A.Az=S47J::Num(J,TEXT("az"),26);A.El=S47J::Num(J,TEXT("el"),40);A.Slew=S47J::Num(J,TEXT("slew"),9);A.Secs=S47J::Num(J,TEXT("hold"));A.Index=S47J::Int(J,TEXT("dish"),-1);break;
  case ES47ScriptAct::Fault:A.Kind=S47J::Str(J,TEXT("kind"),TEXT("omen"));A.Gain=S47J::Num(J,TEXT("severity"),.5);A.Secs=S47J::Num(J,TEXT("secs"),15);break;
  case ES47ScriptAct::Line:A.Who=S47J::Str(J,TEXT("who"),TEXT("dale"));A.Text=S47J::Str(J,TEXT("text"));break;
  case ES47ScriptAct::Toast:A.Text=S47J::Str(J,TEXT("text"));A.Secs=S47J::Num(J,TEXT("secs"),4);break;
  case ES47ScriptAct::Note:A.Title=S47J::Str(J,TEXT("title"));A.Text=S47J::Str(J,TEXT("text"));break;
  case ES47ScriptAct::Evidence:A.Kind=S47J::Str(J,TEXT("id"));A.Text=S47J::Str(J,TEXT("text"));break;
  case ES47ScriptAct::Look:A.Secs=S47J::Num(J,TEXT("secs"),1.5);break;
  case ES47ScriptAct::Wait:A.Secs=S47J::Num(J,TEXT("secs"),5);A.bFlag=S47J::Bool(J,TEXT("until_look"));break;
  case ES47ScriptAct::Shot: // SHOT-BEAT-01: the second eye only after a first
  {
   const FS47ScriptPoint Eye=PointFrom(S47J::Obj(J,TEXT("eye")));
   if(Eye.bSet){A.Path.Add(Eye);const FS47ScriptPoint Eye2=PointFrom(S47J::Obj(J,TEXT("eye2")));if(Eye2.bSet)A.Path.Add(Eye2);}
   A.Fov=S47J::Num(J,TEXT("fov"),60);A.Fov2=S47J::Num(J,TEXT("fov2"),0);
   A.Secs=S47J::Num(J,TEXT("secs"),6);A.In=S47J::Num(J,TEXT("in"),.6);A.Out=S47J::Num(J,TEXT("out"),.8);
   break;
  }
  default:break;
  }
  Out.Actions.Add(A);
 }
 // The timeline in order; the same start keeps the file's order.
 Out.Actions.StableSort([](const FS47ScriptAction& A,const FS47ScriptAction& B){return A.At<B.At;});
 return true;
}

FString Write(const FS47Script& S){return S47J::Write(ToJson(S),true);}
bool Read(const FString& Text,FS47Script& Out,FString& Why){return FromJson(S47J::Read(Text),Out,Why);}

double ParseHour(const FString& Text)
{
 const FString T=Text.TrimStartAndEnd();
 if(T.IsEmpty())return -1;
 FString H,Mi;
 if(T.Split(TEXT(":"),&H,&Mi))
 {
  if(!H.IsNumeric()||!Mi.IsNumeric())return -1;
  const double V=FCString::Atod(*H)+FCString::Atod(*Mi)/60.;
  return V>=0&&V<=24?FMath::Fmod(V,24.):-1;
 }
 if(!T.IsNumeric())return -1;
 const double V=FCString::Atod(*T);
 return V>=0&&V<=24?FMath::Fmod(V,24.):-1;
}

FString HourText(double Hour)
{
 if(Hour<0)return TEXT("--:--");
 const int32 M=FMath::RoundToInt(FMath::Fmod(Hour,24.)*60);
 return FString::Printf(TEXT("%02d:%02d"),(M/60)%24,M%60);
}

FString CleanId(const FString& Text)
{
 FString Out;
 for(const TCHAR C:Text.ToLower())
 {
  if((C>='a'&&C<='z')||(C>='0'&&C<='9')||C=='_')Out.AppendChar(C);
  else if((C==' '||C=='-')&&!Out.IsEmpty()&&!Out.EndsWith(TEXT("_")))Out.AppendChar('_');
  if(Out.Len()>=48)break;
 }
 while(Out.EndsWith(TEXT("_")))Out.LeftChopInline(1);
 return Out;
}

FString LintText(const FString& Text)
{
 using namespace S47ScriptLocal;
 if(Text.IsEmpty())return FString();
 const TArray<FWord> W=Words(Text);
 // A clock with seconds that end on :47 (05:29:47).
 for(int32 I=0;I+2<Text.Len();++I)
  if(Text[I]==':'&&Text[I+1]=='4'&&Text[I+2]=='7'&&(I+3>=Text.Len()||!FChar::IsDigit(Text[I+3]))&&I>=5&&Text[I-3]==':'&&FChar::IsDigit(Text[I-1]))
   return TEXT("a clock ending on :47 points at the 47 seconds (Story's)");
 for(int32 I=0;I<W.Num();++I)
 {
  const FString& L=W[I].Low;
  if(L==TEXT("47")&&Near(W,I,1,{TEXT("s"),TEXT("sec"),TEXT("secs"),TEXT("second"),TEXT("seconds"),TEXT("sekund"),TEXT("sekunder")}))return TEXT("the 47 seconds are Story's");
  if(L==TEXT("forty")&&Near(W,I,1,{TEXT("seven")}))return TEXT("forty-seven points at the 47 seconds or 1947 (Story's)");
  if(L==TEXT("fortyseven")||L==TEXT("1947")||L==TEXT("47s"))return TEXT("1947 and its photograph are Story's");
  if(L==TEXT("47")&&W[I].Start>0&&(Text[W[I].Start-1]=='\''||Text[W[I].Start-1]==0x2019))return TEXT("'47 points at 1947 (Story's)");
  if(W[I].Raw.Equals(TEXT("C"),ESearchCase::CaseSensitive)&&(W[I].Start+1>=Text.Len()||(Text[W[I].Start+1]!='-'&&Text[W[I].Start+1]!='\''&&Text[W[I].Start+1]!=0x2019))&&(W[I].Start==0||(Text[W[I].Start-1]!=0xB0&&!FChar::IsDigit(Text[W[I].Start-1]))))return TEXT("C is Story's");
  if(L==TEXT("c")&&Near(W,I,-1,{TEXT("point"),TEXT("crossing"),TEXT("marker"),TEXT("mark")}))return TEXT("point C is Story's");
  if(StartsAny(L,{TEXT("cable"),TEXT("kabel")})&&(Near(W,I,5,{TEXT("cut"),TEXT("cuts"),TEXT("cutting"),TEXT("severed"),TEXT("snipped"),TEXT("sliced"),TEXT("deliberate"),TEXT("deliberately"),TEXT("kuttet"),TEXT("kutt")})||Near(W,I,-5,{TEXT("cut"),TEXT("cuts"),TEXT("cutting"),TEXT("severed"),TEXT("snipped"),TEXT("sliced"),TEXT("deliberate"),TEXT("deliberately"),TEXT("kuttet"),TEXT("kutt")})))
   return TEXT("the cut cable is Story's");
  if(L==TEXT("tomas"))return TEXT("Tomas is Story's");
  if(StartsAny(L,{TEXT("silhouette"),TEXT("silhuett")})&&Near(W,I,-3,{TEXT("third"),TEXT("three"),TEXT("3rd"),TEXT("tredje"),TEXT("tre")}))return TEXT("the third silhouette is Story's");
  if(AnyOf(L,{TEXT("third"),TEXT("tredje")})&&Near(W,I,2,{TEXT("figure"),TEXT("shape"),TEXT("silhouette"),TEXT("figur")}))return TEXT("the third figure is Story's");
  if(L==TEXT("8.15")||L==TEXT("8,15")||(L==TEXT("eight")&&Near(W,I,1,{TEXT("fifteen")})))return TEXT("8.15 is Story's");
  if(L==TEXT("how")&&Near(W,I,6,{TEXT("taken"),TEXT("abducted"),TEXT("abduction"),TEXT("lifted"),TEXT("beamed"),TEXT("grabbed")}))return TEXT("how anyone was taken is never told");
  if(L==TEXT("took")&&Near(W,I,2,{TEXT("him"),TEXT("her"),TEXT("them"),TEXT("reyes"),TEXT("ortiz"),TEXT("mara")})&&Near(W,I,-4,{TEXT("how"),TEXT("they"),TEXT("it"),TEXT("light"),TEXT("something"),TEXT("what")}))return TEXT("how anyone was taken is never told");
  if(AnyOf(L,{TEXT("reyes"),TEXT("ortiz"),TEXT("mara")})&&(Near(W,I,6,{TEXT("taken"),TEXT("took"),TEXT("abducted"),TEXT("abduction"),TEXT("lifted"),TEXT("beamed"),TEXT("grabbed"),TEXT("pulled")})||Near(W,I,-6,{TEXT("taken"),TEXT("took"),TEXT("abducted"),TEXT("abduction"),TEXT("lifted"),TEXT("beamed"),TEXT("grabbed"),TEXT("pulled")})))
   return TEXT("how Reyes or Ortiz was taken is never told");
  if(L==TEXT("beamed")||(L==TEXT("beam")&&Near(W,I,1,{TEXT("up"),TEXT("him"),TEXT("her"),TEXT("them")})))return TEXT("how anyone was taken is never told");
 }
 return FString();
}

TArray<FString> Lint(const FS47Script& S,TArray<FString>* Warnings)
{
 TArray<FString> E;
 const auto Words=[&E](const FString& Where,const FString& Text){const FString Why=LintText(Text);if(!Why.IsEmpty())E.Add(FString::Printf(TEXT("%s: %s"),*Where,*Why));};
 if(S.Id.IsEmpty()||CleanId(S.Id)!=S.Id)E.Add(TEXT("id: a to z, 0 to 9 and _ only"));
 Words(TEXT("name"),S.Name);
 const FS47ScriptTrigger& T=S.Trigger;
 if(T.DayTo>0&&T.DayFrom>T.DayTo)E.Add(TEXT("trigger: the days run backwards"));
 if(T.NightTo>0&&T.NightFrom>T.NightTo)E.Add(TEXT("trigger: the nights run backwards"));
 if((T.HourFrom<0)!=(T.HourTo<0))E.Add(TEXT("trigger: an hour window needs both ends"));
 for(const FS47ScriptCond& C:T.Conds)
 {
  if(!CondKinds().Contains(C.If))E.Add(FString::Printf(TEXT("condition \"%s\" is not known"),*C.If));
  else if((C.If==TEXT("finding")||C.If==TEXT("no_finding")||C.If==TEXT("near")||C.If==TEXT("far"))&&C.Arg.IsEmpty())E.Add(FString::Printf(TEXT("condition %s needs an id"),*C.If));
  else if(C.If==TEXT("weather")&&!WeatherWords().Contains(C.Arg))E.Add(FString::Printf(TEXT("condition weather: \"%s\" is not a weather word"),*C.Arg));
  else if(C.If.StartsWith(TEXT("case_"))&&(C.Num<0||C.Num>7))E.Add(TEXT("condition case: a case line is 0 to 7"));
 }
 if(S.Actions.IsEmpty())E.Add(TEXT("no actions"));
 // A figure is never only a hint; a glimpse is one that goes when it is looked at (TIERS-20261109).
 if(S.HasFigure()&&S.Tier==ES47Allowed::Hint)E.Add(TEXT("tier: a figure is a glimpse at least"));
 if(S.Tier==ES47Allowed::Glimpse&&S.Actions.ContainsByPredicate([](const FS47ScriptAction& A){return A.Do==ES47ScriptAct::Figure&&!A.bFlag;}))
  E.Add(TEXT("tier: a glimpse vanishes when looked at; every figure needs vanish_seen"));
 for(int32 I=0;I<S.Actions.Num();++I)
 {
  const FS47ScriptAction& A=S.Actions[I];
  const FString Where=FString::Printf(TEXT("action %d (%s)"),I+1,ActName(A.Do));
  Words(Where,A.Text);Words(Where,A.Title);
  switch(A.Do)
  {
  case ES47ScriptAct::Figure:if(!Bodies().Contains(A.Kind))E.Add(Where+TEXT(": not a body")); if(A.Path.IsEmpty())E.Add(Where+TEXT(": no path"));break;
  case ES47ScriptAct::Sky:if(!Forms().Contains(A.Kind))E.Add(Where+TEXT(": not a formation"));break;
  case ES47ScriptAct::Sound:if(A.Text.IsEmpty())E.Add(Where+TEXT(": no recording named"));if(!SoundWhere().Contains(A.Who))E.Add(Where+TEXT(": not a place for a sound"));if(A.Who==TEXT("placed")&&!A.Point.bSet)E.Add(Where+TEXT(": placed, but no point"));break;
  case ES47ScriptAct::Fault:if(!FaultKinds().Contains(A.Kind))E.Add(Where+TEXT(": not a fault"));break;
  case ES47ScriptAct::Dish:if(A.Index<-1||A.Index>26)E.Add(Where+TEXT(": an antenna is 0 to 26, or -1 for all"));break;
  case ES47ScriptAct::Line:if(A.Text.IsEmpty())E.Add(Where+TEXT(": nothing to say"));if(!Voices().Contains(A.Who))E.Add(Where+TEXT(": not a voice"));break;
  case ES47ScriptAct::Toast:case ES47ScriptAct::Note:if(A.Text.IsEmpty())E.Add(Where+TEXT(": no text"));break;
  case ES47ScriptAct::Evidence:if(A.Kind.IsEmpty()||A.Text.IsEmpty())E.Add(Where+TEXT(": a finding needs an id and a text"));break;
  case ES47ScriptAct::Look:if(!A.Point.bSet)E.Add(Where+TEXT(": no point to look at"));break;
  case ES47ScriptAct::Wait:if(A.bFlag&&!A.Point.bSet)E.Add(Where+TEXT(": until looked at, but no point"));break;
  case ES47ScriptAct::Shot:
  {
   // SHOT-BEAT-01: the limits of S47Shot.h (MaxSecs, FovMin/FovMax, LeastFade/MostFade, LeastReach, MostTravel), written as what
   // must hold so that a number that is not one fails. The distances here are road-local with Up; in play each is over its ground.
   if(A.Path.IsEmpty())E.Add(Where+TEXT(": no eye"));
   if(A.Path.Num()>2)E.Add(Where+TEXT(": two eyes at most"));
   if(!A.Point.bSet)E.Add(Where+TEXT(": no point to look at"));
   if(!(A.Secs>=1&&A.Secs<=30))E.Add(Where+TEXT(": held 1 to 30 s"));
   if(!(A.Fov>=20&&A.Fov<=90)||!(A.Fov2==0||(A.Fov2>=20&&A.Fov2<=90)))E.Add(Where+TEXT(": a lens is 20 to 90 degrees (fov2 0: the same)"));
   if(!(A.In>=.3&&A.In<=3&&A.Out>=.3&&A.Out<=3))E.Add(Where+TEXT(": in and out 0.3 to 3 s"));
   const auto Up=[](const FS47ScriptPoint& P){return P.Up>=0&&P.Up<=300;};
   const auto At=[](const FS47ScriptPoint& P){return FVector(P.X,P.Up,P.Z);};
   bool bUp=!A.Point.bSet||Up(A.Point);
   for(const FS47ScriptPoint& P:A.Path)bUp&=Up(P);
   if(!bUp)E.Add(Where+TEXT(": up 0 to 300 m"));
   if(A.Point.bSet&&A.Path.ContainsByPredicate([&At,&A](const FS47ScriptPoint& P){return !(FVector::Dist(At(P),At(A.Point))>=.5);}))E.Add(Where+TEXT(": an eye on the point it looks at (0.5 m at least)"));
   if(A.Path.Num()>1&&!(FVector::Dist(At(A.Path[0]),At(A.Path[1]))<=200))E.Add(Where+TEXT(": the two eyes 200 m apart at most"));
   break;
  }
  default:break;
  }
 }
 if(Warnings)
 {
  Warnings->Reset();
  // Its tier's hour in the first week, and the soonest of any week (the fourth): what a figure waits for.
  const double First=FigureFrom(S,1),Soonest=FigureFrom(S,S47NightTiers::LateFromNight);
  const FString When=FMath::IsNearlyEqual(First,Soonest)?HourText(First):FString::Printf(TEXT("%s (%s in the fourth week)"),*HourText(First),*HourText(Soonest));
  // What holds it to the night's hours: a figure, a tier the file sets, or a formation in the sky (Need, CRIT6-20261109).
  const bool bHeld=S.Need()!=ES47Allowed::Hint;
  const FString What=S.HasFigure()?FString::Printf(TEXT("a figure (%s)"),S47NightTiers::Name(S.Need())):S.bTierSet&&S.Need()==S.Tier?FString::Printf(TEXT("its tier (%s)"),S47NightTiers::Name(S.Need())):FString(TEXT("a light in the sky (a glimpse)"));
  if(bHeld&&T.HourFrom>=0)
  {
   // Does the window reach the hours it may start in at all, in any week? Look at every quarter hour of it.
   bool bAny=false;for(double H=0;H<24&&!bAny;H+=.25)bAny=InHours(H,T.HourFrom,T.HourTo)&&InHours(H,Soonest,AlienUntil);
   if(!bAny)Warnings->Add(FString::Printf(TEXT("%s only starts from %s to %s, and these hours never reach them: it will never play"),*What,*When,*HourText(AlienUntil)));
  }
  if(bHeld&&T.HourFrom<0)Warnings->Add(FString::Printf(TEXT("%s: it waits for %s whatever the trigger says"),*What,*When));
  if(bHeld&&S.bEarly)Warnings->Add(FString::Printf(TEXT("early: set on purpose, it may start from %s"),*HourText(AlienEarlyFrom)));
  // CRIT6-20261109: a tier and no figure: it now holds what the script does (its sounds, lines and findings) to that tier's hours.
  if(S.bTierSet&&!S.HasFigure())Warnings->Add(S.Tier==ES47Allowed::Hint
   ?(S.HasSky()?FString(TEXT("tier hint, but a light in the sky is a glimpse at least: it waits for the glimpse's hour")):FString(TEXT("tier hint and no figure: it plays at any hour its trigger allows")))
   :FString::Printf(TEXT("tier %s and no figure: everything it does waits for %s"),S47NightTiers::Name(S.Tier),*When));
  if(!S.bTierSet&&!S.HasFigure()&&S.HasSky())Warnings->Add(FString::Printf(TEXT("no tier set, but a light in the sky is a glimpse at least: it waits for %s"),*When));
  if(!S.bEnabled)Warnings->Add(TEXT("switched off: it plays only from PREVIEW"));
 }
 return E;
}

double FigureFrom(const FS47Script& S,int32 Night)
{
 return S.bEarly?AlienEarlyFrom:S47NightTiers::FromClockHour(S.Need(),Night);
}

bool InHours(double Hour,double From,double To)
{
 if(From<0||To<0)return true;
 if(FMath::IsNearlyEqual(From,To))return true;
 return From<To?(Hour>=From&&Hour<To):(Hour>=From||Hour<To);
}

bool CondHolds(const FS47ScriptCond& C,const FS47ScriptNow& Now)
{
 const FString& I=C.If;
 const auto Found=[&Now](const FString& Id)
 {
  if(!Now.HasFinding)return false;
  return Now.HasFinding(Id);
 };
 if(I==TEXT("finding"))return Found(C.Arg);
 if(I==TEXT("no_finding"))return !Found(C.Arg);
 if(I.StartsWith(TEXT("case_")))
 {
  const int32 St=Now.CaseState?Now.CaseState(int32(C.Num)):0;
  return I==TEXT("case_open")?St==1:I==TEXT("case_closed")?St==2:St==0;
 }
 if(I==TEXT("weather"))return Now.Weather.Contains(C.Arg);
 if(I==TEXT("moon_min"))return Now.MoonLit>=C.Num;
 if(I==TEXT("moon_max"))return Now.MoonLit<=C.Num;
 if(I==TEXT("moon_up"))return Now.bMoonUp;
 if(I==TEXT("indoors"))return Now.bIndoors;
 if(I==TEXT("outdoors"))return !Now.bIndoors;
 if(I==TEXT("in_truck"))return Now.bInTruck;
 if(I==TEXT("on_foot"))return !Now.bInTruck;
 if(I==TEXT("near")||I==TEXT("far"))
 {
  FVector2D At;double R=0;
  if(!Now.bHasRoad||!Now.PlaceAt||!Now.PlaceAt(C.Arg,At,R))return false;
  const double D=FVector2D::Distance(At,Now.Road);
  return I==TEXT("near")?D<=C.Num:D>=C.Num;
 }
 return false;
}

bool Ready(const FS47Script& S,const FS47ScriptNow& Now,const FS47ScriptMemory& Mem,FString& Why,bool bPreview)
{
 Why.Reset();
 const TArray<FString> Errors=Lint(S);
 if(Errors.Num()){Why=Errors[0];return false;}
 // The night's tier (S47NightTiers): a full figure from two, a glimpse from midnight, an hour sooner in the fourth week; early, from
 // the evening, only when the file says so.
 const double From=FigureFrom(S,Now.Night);
 // CRIT6-20261109: what a script needs is its Need (a figure, a tier set, a light in the sky); a plain hint at any hour.
 const bool bAlienHours=S.Need()==ES47Allowed::Hint||InHours(Now.Hour(),From,AlienUntil);
 if(bPreview)
 {
  if(!bAlienHours)Why=FString::Printf(TEXT("in play this waits for %s (%s, %s)"),*HourText(From),S.HasFigure()?TEXT("a figure"):TEXT("its tier"),S47NightTiers::Name(S.Need()));
  if(Now.bBigEvent)Why+=(Why.IsEmpty()?FString():FString(TEXT("; ")))+TEXT("in play this waits: ")+Now.BigWhy;
  return true;
 }
 if(!S.bEnabled){Why=TEXT("switched off");return false;}
 const int32 Times=Mem.Count.FindRef(S.Id);
 if(S.Repeat==ES47ScriptRepeat::Once&&Times>0){Why=TEXT("played once already");return false;}
 if(S.Repeat==ES47ScriptRepeat::Nightly&&Times>0&&Mem.LastNight.FindRef(S.Id)==Now.Night){Why=TEXT("played tonight already");return false;}
 if(S.Repeat==ES47ScriptRepeat::Always&&Times>0&&Now.Clock-Mem.LastClock.FindRef(S.Id)<S.CooldownMin*60){Why=TEXT("cooling down");return false;}
 const FS47ScriptTrigger& T=S.Trigger;
 if(T.DayFrom>0&&Now.Day<T.DayFrom){Why=TEXT("too early a day");return false;}
 if(T.DayTo>0&&Now.Day>T.DayTo){Why=TEXT("its days are over");return false;}
 if(T.NightFrom>0&&Now.Night<T.NightFrom){Why=TEXT("too early a night");return false;}
 if(T.NightTo>0&&Now.Night>T.NightTo){Why=TEXT("its nights are over");return false;}
 if(!InHours(Now.Hour(),T.HourFrom,T.HourTo)){Why=TEXT("outside its hours");return false;}
 if(!bAlienHours){Why=FString::Printf(TEXT("%s (%s) waits for %s"),S.HasFigure()?TEXT("a figure"):TEXT("its tier"),S47NightTiers::Name(S.Need()),*HourText(From));return false;}
 if(T.HasWhere())
 {
  if(!Now.bHasRoad){Why=TEXT("no player on the land");return false;}
  FVector2D At=T.Point.Road();double R=0;
  if(!T.Place.IsEmpty()&&!(Now.PlaceAt&&Now.PlaceAt(T.Place,At,R))){Why=TEXT("its place is not on the list");return false;}
  if(FVector2D::Distance(At,Now.Road)>T.Radius){Why=TEXT("not at its place");return false;}
 }
 for(const FS47ScriptCond& C:T.Conds)if(!CondHolds(C,Now)){Why=FString::Printf(TEXT("condition %s"),*C.If);return false;}
 if(Now.bBigEvent){Why=TEXT("waits: ")+Now.BigWhy;return false;}
 return true;
}
}
