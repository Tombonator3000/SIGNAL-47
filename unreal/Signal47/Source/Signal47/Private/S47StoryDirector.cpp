#include "S47StoryDirector.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Script.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// STORY-ON-FREEFORM-01, step 3: see S47StoryDirector.h.
DEFINE_LOG_CATEGORY_STATIC(LogS47StoryDirector,Log,All);

FString FS47StoryDirector::ActsPath()
{
 return FPaths::ProjectContentDir()/TEXT("Director/Story/acts.json");
}

bool FS47StoryDirector::ParseFrom(const FString& Text,double& OutHour)
{
 if(Text.Len()!=5||Text[2]!=TEXT(':'))return false;
 for(const int32 I:{0,1,3,4})if(!FChar::IsDigit(Text[I]))return false;
 const int32 Hours=(Text[0]-TEXT('0'))*10+(Text[1]-TEXT('0'));
 const int32 Minutes=(Text[3]-TEXT('0'))*10+(Text[4]-TEXT('0'));
 if(Hours>23||Minutes>59)return false;
 const double At=Hours+Minutes/60.;
 OutHour=At<8.?At+24.:At;
 return true;
}

double FS47StoryDirector::NightHourOf(double Clock)
{
 double T=FMath::Fmod(Clock,86400.);
 if(T<0)T+=86400.;
 const double Hour=T/3600.;
 return Hour<8.?Hour+24.:Hour;
}

bool FS47StoryDirector::ValidId(const FString& Id)
{
 if(Id.IsEmpty()||Id.Len()>MaxIdLen)return false;
 for(const TCHAR C:Id)if(!((C>='a'&&C<='z')||(C>='0'&&C<='9')||C=='_'))return false;
 return true;
}

bool FS47StoryDirector::ParseActs(const FString& Text,TArray<FAct>& Out,FString& Why)
{
 Out.Reset();Why.Reset();
 const S47J::FObj Root=S47J::Read(Text);
 if(!Root.IsValid()){Why=TEXT("not a JSON object");return false;}
 if(S47J::Int(Root,TEXT("v"),0)!=1){Why=TEXT("v is not 1");return false;}
 const TArray<TSharedPtr<FJsonValue>> List=S47J::Arr(Root,TEXT("acts"));
 if(List.IsEmpty()){Why=TEXT("no acts");return false;}
 TArray<FAct> Parsed;
 for(int32 I=0;I<List.Num();++I)
 {
  const TSharedPtr<FJsonObject>* Entry=nullptr;
  if(!List[I].IsValid()||List[I]->Type!=EJson::Object||!List[I]->TryGetObject(Entry)||!Entry||!Entry->IsValid())
  {Why=FString::Printf(TEXT("act %d is not an object"),I);return false;}
  const S47J::FObj A=*Entry;
  FAct Act;
  Act.Id=S47J::Str(A,TEXT("id"));
  Act.Title=S47J::Str(A,TEXT("title"));
  if(!ValidId(Act.Id)){Why=FString::Printf(TEXT("act %d: id \"%s\" is not 1 to %d of a-z, 0-9 and _"),I,*Act.Id,MaxIdLen);return false;}
  for(const FAct& Earlier:Parsed)if(Earlier.Id==Act.Id){Why=FString::Printf(TEXT("act %d: id \"%s\" is used twice"),I,*Act.Id);return false;}
  const TArray<FString> TitleLint=LintStory(Act.Title);
  if(!TitleLint.IsEmpty()){Why=FString::Printf(TEXT("act %s: title %s"),*Act.Id,*TitleLint[0]);return false;}
  if(!ParseFrom(S47J::Str(A,TEXT("from")),Act.From)){Why=FString::Printf(TEXT("act %s: from is not HH:MM"),*Act.Id);return false;}
  if(Parsed.Num()>0&&Act.From<Parsed.Last().From){Why=FString::Printf(TEXT("act %s starts before the act ahead of it"),*Act.Id);return false;}
  for(const TSharedPtr<FJsonValue>& V:S47J::Arr(A,TEXT("needs")))
  {
   FString Need;
   if(!V.IsValid()||V->Type!=EJson::String||!V->TryGetString(Need)||!ValidId(Need)){Why=FString::Printf(TEXT("act %s: a need is not a flag id"),*Act.Id);return false;}
   Act.Needs.AddUnique(Need);
  }
  Parsed.Add(MoveTemp(Act));
 }
 Out=MoveTemp(Parsed);
 return true;
}

TArray<FString> FS47StoryDirector::LintStory(const FString& Text)
{
 TArray<FString> Out;
 if(Text.TrimStartAndEnd().IsEmpty()){Out.Add(TEXT("is empty"));return Out;}
 if(Text.Len()>MaxStoryText)Out.Add(FString::Printf(TEXT("is longer than %d characters"),MaxStoryText));
 for(const TCHAR C:Text)
  if((C<0x20&&C!=TEXT('\n'))||(C>=0x7F&&C<=0x9F)||C==0xFFFD||(C>=0xD800&&C<=0xDFFF))
  {Out.Add(TEXT("has a character that does not print"));break;}
 // A short list: real people and other works the game must never name (Dale's temper is his own; no extracted game is used).
 static const TCHAR* const Banned[]={TEXT("kinski"),TEXT("greyhill"),TEXT("x-files"),TEXT("mulder"),TEXT("scully"),TEXT("spielberg"),TEXT("coca-cola"),TEXT("pepsi")};
 const FString Low=Text.ToLower();
 for(const TCHAR* Word:Banned)if(Low.Contains(Word)){Out.Add(FString::Printf(TEXT("names \"%s\", which the game must not use"),Word));break;}
 return Out;
}

void FS47StoryDirector::Begin(const S47J::FObj& Saved)
{
 FString Text;
 const FString Path=ActsPath();
 if(!FFileHelper::LoadFileToString(Text,*Path))
 {
  Stop();
  UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story director: no acts at %s; inactive"),*Path);
  return;
 }
 BeginWith(Text,Saved);
}

void FS47StoryDirector::BeginWith(const FString& ActsText,const S47J::FObj& Saved)
{
 Stop();
 TArray<FAct> Parsed;FString Why;
 if(!ParseActs(ActsText,Parsed,Why))
 {
  UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story director: acts are not valid (%s); inactive"),*Why);
  return;
 }
 ActList=MoveTemp(Parsed);
 bActive=true;
 Load(Saved);
 UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story act %s (%s)"),*ActList[ActIndex].Id,*StateText());
}

void FS47StoryDirector::Load(const S47J::FObj& Saved)
{
 ActIndex=0;Flags.Reset();
 if(!Saved.IsValid()||S47J::Int(Saved,TEXT("v"),0)!=1)return; // no save, or one this version cannot read: the first act
 const FString At=S47J::Str(Saved,TEXT("act"));
 for(int32 I=0;I<ActList.Num();++I)if(ActList[I].Id==At){ActIndex=I;break;}
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(Saved,TEXT("flags")))
 {
  FString Flag;
  if(V.IsValid()&&V->Type==EJson::String&&V->TryGetString(Flag)&&ValidId(Flag))Flags.Add(Flag);
 }
}

void FS47StoryDirector::Stop()
{
 bActive=false;
 ActList.Reset();
 ActIndex=-1;
 Flags.Reset();
}

bool FS47StoryDirector::ClockNow(double& Out) const
{
 if(ClockOverride>=0){Out=ClockOverride;return true;}
 if(M&&M->Freeform){Out=M->Freeform->Clock();return true;}
 return false;
}

void FS47StoryDirector::Tick(double)
{
 if(!bActive||!ActList.IsValidIndex(ActIndex))return;
 double Clock=0;
 if(!ClockNow(Clock))return;
 const double Hour=NightHourOf(Clock);
 while(ActList.IsValidIndex(ActIndex+1))
 {
  const FAct& Next=ActList[ActIndex+1];
  if(Hour<Next.From)break;
  bool bNeedsSet=true;
  for(const FString& Need:Next.Needs)bNeedsSet&=Flags.Contains(Need);
  if(!bNeedsSet)break;
  ++ActIndex;
  UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story act %s"),*Next.Id);
 }
}

S47J::FObj FS47StoryDirector::Save() const
{
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("v"),1);
 if(const FAct* Now=CurrentAct())S47J::Set(O,TEXT("act"),Now->Id);
 S47J::Set(O,TEXT("flags"),FlagList());
 return O;
}

bool FS47StoryDirector::SetFlag(const FString& Flag)
{
 if(!ValidId(Flag))return false;
 Flags.Add(Flag);
 return true;
}

bool FS47StoryDirector::ClearFlag(const FString& Flag)
{
 return Flags.Remove(Flag)>0;
}

TArray<FString> FS47StoryDirector::FlagList() const
{
 TArray<FString> Out=Flags.Array();
 Out.Sort();
 return Out;
}

FString FS47StoryDirector::StateText() const
{
 const FAct* Now=CurrentAct();
 if(!Now)return TEXT("inactive");
 return FString::Printf(TEXT("act %s, %d of %d, flags [%s]"),*Now->Id,ActIndex+1,ActList.Num(),*FString::Join(FlagList(),TEXT(",")));
}

bool FS47StoryDirector::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Name){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+Name);};
 const FString ActsText=TEXT("{\"v\":1,\"acts\":[{\"id\":\"evening\",\"title\":\"Evening\",\"from\":\"18:00\"},")
  TEXT("{\"id\":\"night\",\"title\":\"Night\",\"from\":\"00:00\",\"needs\":[\"met_dale\"]},{\"id\":\"morning\",\"title\":\"Morning\",\"from\":\"04:00\"}]}");
 const double Day2=86400.;
 { // the acts parse, in order, with the night's hours
  TArray<FAct> Parsed;FString Why;
  const bool bParsed=ParseActs(ActsText,Parsed,Why);
  Check(bParsed&&Parsed.Num()==3&&Parsed[0].Id==TEXT("evening")&&Parsed[0].From==18.&&Parsed[1].From==24.&&Parsed[2].From==28.&&Parsed[1].Needs.Num()==1&&Parsed[1].Needs[0]==TEXT("met_dale")&&Parsed[2].Needs.IsEmpty(),
   TEXT("STORY-3: acts parse with id, title, the night's hour and needs"));
  double Hour=0;
  const bool bFrom=ParseFrom(TEXT("18:30"),Hour)&&FMath::IsNearlyEqual(Hour,18.5)&&ParseFrom(TEXT("00:00"),Hour)&&Hour==24.&&ParseFrom(TEXT("07:59"),Hour)&&Hour>31.9
   &&!ParseFrom(TEXT("24:00"),Hour)&&!ParseFrom(TEXT("7:00"),Hour)&&!ParseFrom(TEXT("ab:cd"),Hour)&&!ParseFrom(FString(),Hour);
  Check(bFrom&&FMath::IsNearlyEqual(NightHourOf(64800.),18.)&&FMath::IsNearlyEqual(NightHourOf(Day2+1800.),24.5)&&FMath::IsNearlyEqual(NightHourOf(Day2+9*3600.),9.),
   TEXT("STORY-3: HH:MM and the clock read as the night's hour (after midnight is 24 and over)"));
  TArray<FAct> Bad;FString BadWhy;
  const bool bRejects=!ParseActs(TEXT("not json"),Bad,BadWhy)&&!ParseActs(TEXT("{\"v\":1,\"acts\":[]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":2,\"acts\":[{\"id\":\"a\",\"title\":\"A\",\"from\":\"18:00\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"a\",\"title\":\"A\",\"from\":\"01:00\"},{\"id\":\"b\",\"title\":\"B\",\"from\":\"18:00\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"a\",\"title\":\"A\",\"from\":\"18:00\"},{\"id\":\"a\",\"title\":\"B\",\"from\":\"19:00\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"Bad Id\",\"title\":\"A\",\"from\":\"18:00\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"a\",\"title\":\"\",\"from\":\"18:00\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"a\",\"title\":\"A\",\"from\":\"6 pm\"}]}"),Bad,BadWhy)
   &&!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"a\",\"title\":\"A\",\"from\":\"18:00\",\"needs\":[3]}]}"),Bad,BadWhy)&&Bad.IsEmpty();
  Check(bRejects,TEXT("STORY-3: bad acts are refused whole (not JSON, none, version, order, twice, id, title, from, needs)"));
 }
 { // an act starts when its hour has come and its needs are set; never back, never the clock moved
  FS47StoryDirector D(nullptr);
  D.BeginWith(ActsText,S47J::FObj());
  const bool bFirst=D.Active()&&D.CurrentAct()&&D.CurrentAct()->Id==TEXT("evening");
  D.TestClock(64800.);D.Tick(.1);
  const bool bEvening=D.CurrentIndex()==0;
  D.TestClock(Day2-60.);D.Tick(.1);
  const bool bBeforeMidnight=D.CurrentIndex()==0;
  D.TestClock(Day2+1800.);D.Tick(.1);
  const bool bWaitsForNeeds=D.CurrentIndex()==0;
  const bool bSet=D.SetFlag(TEXT("met_dale"))&&!D.SetFlag(TEXT("Not A Flag"))&&D.HasFlag(TEXT("met_dale"));
  D.Tick(.1);
  const bool bNight=D.CurrentAct()&&D.CurrentAct()->Id==TEXT("night");
  D.TestClock(Day2+4*3600.);D.Tick(.1);D.Tick(.1);
  const bool bMorning=D.CurrentAct()&&D.CurrentAct()->Id==TEXT("morning");
  D.TestClock(Day2+3600.);D.Tick(.1);
  const bool bNeverBack=D.CurrentIndex()==2;
  Check(bFirst&&bEvening&&bBeforeMidnight&&bWaitsForNeeds&&bSet&&bNight&&bMorning&&bNeverBack,TEXT("STORY-3: acts advance by the night's clock and by their needs, and never go back"));
  FS47StoryDirector Late(nullptr);
  Late.BeginWith(ActsText,S47J::FObj());Late.SetFlag(TEXT("met_dale"));Late.TestClock(Day2+5*3600.);Late.Tick(.1);
  Check(Late.CurrentIndex()==2,TEXT("STORY-3: every act that is due starts in one step"));
  const bool bCleared=D.ClearFlag(TEXT("met_dale"))&&!D.HasFlag(TEXT("met_dale"))&&!D.ClearFlag(TEXT("met_dale"));
  D.SetFlag(TEXT("met_dale"));D.SetFlag(TEXT("a_flag"));
  { // save and load
   const S47J::FObj Saved=S47J::Read(S47J::Write(D.Save()));
   FS47StoryDirector Back(nullptr);
   Back.BeginWith(ActsText,Saved);
   Check(bCleared&&S47J::Int(Saved,TEXT("v"))==1&&S47J::Str(Saved,TEXT("act"))==TEXT("morning")&&Back.CurrentIndex()==2&&Back.HasFlag(TEXT("met_dale"))&&Back.HasFlag(TEXT("a_flag"))&&Back.FlagList().Num()==2,
    TEXT("STORY-3: the act and the flags keep through a save (v:1)"));
  }
 }
 { // an unknown or broken save starts at the first act
  bool bFallback=true;
  for(const TCHAR* Saved:{TEXT("{\"v\":2,\"act\":\"morning\",\"flags\":[\"met_dale\"]}"),TEXT("{\"v\":1,\"act\":\"nowhere\"}"),TEXT("{\"v\":1}"),TEXT("{\"act\":\"morning\"}")})
  {
   FS47StoryDirector D(nullptr);
   D.BeginWith(ActsText,S47J::Read(Saved));
   bFallback&=D.Active()&&D.CurrentIndex()==0;
  }
  FS47StoryDirector Null(nullptr);Null.BeginWith(ActsText,S47J::FObj());
  FS47StoryDirector Junk(nullptr);Junk.BeginWith(ActsText,S47J::Read(TEXT("{\"v\":1,\"act\":\"night\",\"flags\":[\"Bad Flag\",\"ok_flag\",4]}")));
  Check(bFallback&&Null.CurrentIndex()==0&&Junk.CurrentIndex()==1&&Junk.FlagList().Num()==1&&Junk.HasFlag(TEXT("ok_flag")),TEXT("STORY-3: an unknown, broken or missing save starts at the first act; bad flags are dropped"));
 }
 { // no acts: inactive, and nothing breaks
  FS47StoryDirector D(nullptr);
  D.BeginWith(FString(),S47J::FObj());
  D.TestClock(Day2+5*3600.);D.Tick(.1);
  const S47J::FObj Saved=D.Save();
  Check(!D.Active()&&!D.CurrentAct()&&S47J::Int(Saved,TEXT("v"))==1&&!S47J::Has(Saved,TEXT("act")),TEXT("STORY-3: without valid acts the director is inactive and still saves v:1"));
 }
 { // Story's own text check, and Freeform's left as it was
  FString Bell=TEXT("ab");Bell[1]=TCHAR(7);
  const bool bRefuses=!LintStory(FString()).IsEmpty()&&!LintStory(TEXT("   ")).IsEmpty()&&!LintStory(FString::ChrN(MaxStoryText+1,TEXT('a'))).IsEmpty()
   &&!LintStory(Bell).IsEmpty()&&!LintStory(TEXT("He shouts like Kinski.")).IsEmpty();
  const FString Spoiler=TEXT("The tape stops at 05:29:47, as it did in 1947.");
  const bool bStoryMay=LintStory(Spoiler).IsEmpty()&&LintStory(TEXT("Line one.\nLine two.")).IsEmpty()&&LintStory(FString::ChrN(MaxStoryText,TEXT('a'))).IsEmpty();
  Check(bRefuses&&bStoryMay,TEXT("STORY-3: LintStory refuses empty, too long, unprintable and banned names, and lets the story's own words through"));
  Check(!S47Script::LintText(Spoiler).IsEmpty()&&S47Script::LintText(TEXT("Tomas was here."))==TEXT("Tomas is Story's")&&S47Script::LintText(TEXT("The dish turned north at ten.")).IsEmpty(),
   TEXT("STORY-3: Freeform's LintText still refuses Story's words"));
 }
 { // the acts beside the build
  FString Text;
  if(!FFileHelper::LoadFileToString(Text,*ActsPath()))Lines.Add(TEXT("NOTE story director: no Content/Director/Story/acts.json beside this build"));
  else
  {
   TArray<FAct> Parsed;FString Why;
   bool bClean=ParseActs(Text,Parsed,Why);
   for(const FAct& Act:Parsed)bClean&=S47Script::LintText(Act.Title).IsEmpty();
   Check(bClean&&Parsed.Num()==3&&Parsed[0].Id==TEXT("evening")&&Parsed[1].Id==TEXT("night")&&Parsed[2].Id==TEXT("morning"),TEXT("STORY-3: Content/Director/Story/acts.json has evening, night and morning with neutral titles"));
   if(!Why.IsEmpty())Lines.Add(TEXT("NOTE acts.json: ")+Why);
  }
 }
 return OK;
}
