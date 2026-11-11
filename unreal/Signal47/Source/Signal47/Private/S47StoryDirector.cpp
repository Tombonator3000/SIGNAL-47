#include "S47StoryDirector.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Script.h"
#include "S47DevEvents.h"
#include "S47Flow.h"
#include "S47Happenings.h"
#include "S47ScriptPlayer.h"
#include "S47UI.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// STORY-ON-FREEFORM-01, steps 3 and 4: see S47StoryDirector.h.
DEFINE_LOG_CATEGORY_STATIC(LogS47StoryDirector,Log,All);

namespace S47StoryDirectorLocal
{
 // The story's scripts, read once and again on F10's "Read the story scripts again" (and at each Story night's Begin).
 struct FStore{TArray<FS47EventScript> List;TMap<FString,FString> Bad;bool bRead=false;};
 FStore& Store(){static FStore St;return St;}
 void ReadOnce(){if(!Store().bRead)FS47StoryDirector::ReloadScripts();}
 const TCHAR* const DevGroup=TEXT("Story (director)");
 // The night's hour (8 to 32) as HH:MM.
 FString HourText(double Hour)
 {
  const int32 Minutes=FMath::RoundToInt((Hour>=24?Hour-24:Hour)*60.);
  return FString::Printf(TEXT("%02d:%02d"),(Minutes/60)%24,Minutes%60);
 }
}

FS47StoryDirector::FS47StoryDirector(US47Main* Main):M(Main),Runner(MakeUnique<FS47EventBeats>(Main)){}

FS47StoryDirector::~FS47StoryDirector()
{
 SetHold(false);
}

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
  for(const TSharedPtr<FJsonValue>& V:S47J::Arr(A,TEXT("scripts"))) // step 4: the story scripts the act starts
  {
   FString Script;
   if(!V.IsValid()||V->Type!=EJson::String||!V->TryGetString(Script)||!ValidId(Script)){Why=FString::Printf(TEXT("act %s: a script is not a story script's id"),*Act.Id);return false;}
   Act.Scripts.AddUnique(Script);
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
 ReloadScripts(); // step 4: the story's scripts as they are on disk tonight
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
 // A night that had not reached its act's start yet (a new night, or a save from before step 4) starts the act's scripts now.
 if(!Load(Saved))QueueAct(ActIndex);
 UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story act %s (%s)"),*ActList[ActIndex].Id,*StateText());
 AddDevLines();
}

bool FS47StoryDirector::Load(const S47J::FObj& Saved)
{
 ActIndex=0;Flags.Reset();ObjectiveLine.Reset();Pending.Reset();
 if(!Saved.IsValid()||S47J::Int(Saved,TEXT("v"),0)!=1)return false; // no save, or one this version cannot read: the first act
 bool bAt=false;
 const FString At=S47J::Str(Saved,TEXT("act"));
 for(int32 I=0;I<ActList.Num();++I)if(ActList[I].Id==At){ActIndex=I;bAt=true;break;}
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(Saved,TEXT("flags")))
 {
  FString Flag;
  if(V.IsValid()&&V->Type==EJson::String&&V->TryGetString(Flag)&&ValidId(Flag))Flags.Add(Flag);
 }
 // Step 4 (absent in older saves: none): the objective, and the story scripts under way, taken up where they were.
 const FString Line=S47J::Str(Saved,TEXT("objective"));
 if(!Line.IsEmpty()&&LintStory(Line).IsEmpty())ObjectiveLine=Line;
 for(const TSharedPtr<FJsonValue>& V:S47J::Arr(Saved,TEXT("scripts")))
 {
  const S47J::FObj* Entry=nullptr;
  if(!V.IsValid()||V->Type!=EJson::Object||!V->TryGetObject(Entry)||!Entry||!Entry->IsValid())continue;
  const FString Id=S47J::Str(*Entry,TEXT("id"));
  const double T=S47J::Num(*Entry,TEXT("t"));
  const int32 Played=S47J::Int(*Entry,TEXT("next"));
  if(S47Script::CleanId(Id)!=Id||Id.IsEmpty()||!FMath::IsFinite(T)||T<0||Played<0)continue;
  if(!Pending.ContainsByPredicate([&Id](const FPending& P){return P.Id==Id;}))Pending.Add(FPending{Id,T,Played});
 }
 return bAt;
}

void FS47StoryDirector::Stop()
{
 bActive=false;
 ActList.Reset();
 ActIndex=-1;
 Flags.Reset();
 // Step 4: the story's runs, the objective and a waiting clock go with it, and Freeform's dice are let go.
 if(Runner)Runner->Stop();
 Pending.Reset();
 ObjectiveLine.Reset();
 ClockAt=-1;ClockWaited=0;
 SetHold(false);
}

bool FS47StoryDirector::ClockNow(double& Out) const
{
 if(ClockOverride>=0){Out=ClockOverride;return true;}
 if(M&&M->Freeform){Out=M->Freeform->Clock();return true;}
 return false;
}

void FS47StoryDirector::Tick(double Dt)
{
 if(!bActive||!ActList.IsValidIndex(ActIndex))return;
 double Clock=0;
 if(ClockNow(Clock))
 {
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
   QueueAct(ActIndex);
  }
 }
 // Step 4: the story's scripts, held where they are while a menu, a paper or a held scene is up (as Freeform's events are).
 const bool bSceneHeld=M&&M->Freeform&&M->Freeform->Held();
 if(!bSceneHeld&&Dt>0)
 {
  StartPending();
  if(Runner)Runner->Step(Dt);
 }
 StepClock(Dt);
 SetHold(M!=nullptr&&Busy());
}

S47J::FObj FS47StoryDirector::Save() const
{
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("v"),1);
 if(const FAct* Now=CurrentAct())S47J::Set(O,TEXT("act"),Now->Id);
 S47J::Set(O,TEXT("flags"),FlagList());
 // Step 4: the objective, and the story scripts under way (and those about to start) with how far they had come.
 if(!ObjectiveLine.IsEmpty())S47J::Set(O,TEXT("objective"),ObjectiveLine);
 TArray<TSharedPtr<FJsonValue>> Under;
 const auto Keep=[&Under](const FString& Id,double T,int32 Played)
 {
  const S47J::FObj E=S47J::New();
  S47J::Set(E,TEXT("id"),Id);S47J::Set(E,TEXT("t"),T);S47J::Set(E,TEXT("next"),Played);
  Under.Add(MakeShared<FJsonValueObject>(E));
 };
 if(Runner)for(const FS47EventBeats::FProgress& P:Runner->Progress())Keep(P.Id,P.T,P.Next);
 for(const FPending& P:Pending)Keep(P.Id,P.T,P.Next);
 if(Under.Num())O->SetArrayField(TEXT("scripts"),Under);
 return O;
}

// ---------- step 4: the story's scripts ----------

FString FS47StoryDirector::ScriptsDir()
{
 return FPaths::ProjectContentDir()/TEXT("Director/Story/beats");
}

bool FS47StoryDirector::ReadScript(const FString& Text,FS47EventScript& Out,FString& Why)
{
 if(!S47EventScript::Read(Text,Out,Why))return false;
 const TArray<FString> Errors=S47EventScript::LintStory(Out);
 if(Errors.Num()){Why=Errors[0];return false;}
 return true;
}

void FS47StoryDirector::ReloadScripts()
{
 using namespace S47StoryDirectorLocal;
 FStore& St=Store();
 St.bRead=true;St.List.Reset();St.Bad.Reset();
 const FString Dir=ScriptsDir();
 TArray<FString> Files;
 IFileManager::Get().FindFiles(Files,*(Dir/TEXT("*.json")),true,false);
 Files.Sort();
 for(const FString& File:Files)
 {
  const FString Path=Dir/File;
  FString Text,Why;FS47EventScript Script;
  if(!FFileHelper::LoadFileToString(Text,*Path)){St.Bad.Add(Path,TEXT("could not be read"));UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story script %s could not be read"),*Path);continue;}
  if(!ReadScript(Text,Script,Why)){St.Bad.Add(Path,Why);UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story script %s refused: %s"),*Path,*Why);continue;}
  if(!Script.bEnabled){UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story script %s is switched off"),*Path);continue;}
  const FString Id=Script.Id;
  if(St.List.ContainsByPredicate([&Id](const FS47EventScript& Other){return Other.Id==Id;}))
  {St.Bad.Add(Path,TEXT("its id is another story script's"));UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story script %s refused: id %s is used twice"),*Path,*Id);continue;}
  Script.Source=Path;
  St.List.Add(MoveTemp(Script));
 }
 UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story scripts: %d from %s, %d refused"),St.List.Num(),*Dir,St.Bad.Num());
}

const TArray<FS47EventScript>& FS47StoryDirector::StoryScripts(){S47StoryDirectorLocal::ReadOnce();return S47StoryDirectorLocal::Store().List;}
const FS47EventScript* FS47StoryDirector::FindScript(const FString& Id){return StoryScripts().FindByPredicate([&Id](const FS47EventScript& Script){return Script.Id==Id;});}
const TMap<FString,FString>& FS47StoryDirector::RefusedScripts(){S47StoryDirectorLocal::ReadOnce();return S47StoryDirectorLocal::Store().Bad;}

FS47StoryDirector* FS47StoryDirector::Of(const US47Main* Main)
{
 if(!Main||!Main->IsFreeform()||!Main->Flow||!Main->Flow->StoryNight()||!Main->StoryDirector||!Main->StoryDirector->Active())return nullptr;
 return Main->StoryDirector.Get();
}

bool FS47StoryDirector::Holds(const US47Main* Main)
{
 const FS47StoryDirector* D=Of(Main);
 return D&&D->Busy();
}

bool FS47StoryDirector::FlagCond(const FS47ScriptCond& C) const
{
 const bool bHas=Flags.Contains(C.Arg);
 return C.If==TEXT("no_story_flag")?!bHas:C.If==TEXT("story_flag")&&bHas;
}

bool FS47StoryDirector::CondHolds(const US47Main* Main,const FS47ScriptCond& C)
{
 const FS47StoryDirector* D=Of(Main);
 return D?D->FlagCond(C):C.If==TEXT("no_story_flag");
}

FString FS47StoryDirector::ObjectiveOf(const US47Main* Main)
{
 const FS47StoryDirector* D=Of(Main);
 return D?D->ObjectiveLine:FString();
}

double FS47StoryDirector::ClockPushFor(double Clock,double AtLeast)
{
 if(!FMath::IsFinite(Clock)||!FMath::IsFinite(AtLeast)||AtLeast<8.||AtLeast>=32.)return 0;
 const double Hour=NightHourOf(Clock);
 return AtLeast>Hour?(AtLeast-Hour)*3600.:0.;
}

bool FS47StoryDirector::Busy() const
{
 return Pending.Num()>0||ClockAt>=0||(Runner&&Runner->Live()>0);
}

int32 FS47StoryDirector::RunningCount() const
{
 return Runner?Runner->Live():0;
}

FString FS47StoryDirector::Apply(const FS47Beat& B,const FString& Text)
{
 switch(B.Do)
 {
 case ES47Beat::Flag:
  if(B.bSet)return SetFlag(B.Flag)?TEXT("story flag set: ")+B.Flag:TEXT("not a story flag's id: ")+B.Flag;
  return ClearFlag(B.Flag)?TEXT("story flag cleared: ")+B.Flag:TEXT("story flag was not set: ")+B.Flag;
 case ES47Beat::Start:
 {
  FString Why;
  if(!B.Script.IsEmpty())return StartScript(B.Script,Why)?TEXT("story script starts: ")+B.Script:TEXT("story script not started: ")+B.Script+TEXT(" (")+Why+TEXT(")");
  FS47Happenings* Haps=M&&M->Freeform?M->Freeform->Happenings.Get():nullptr;
  if(!Haps)return TEXT("happening not asked for: ")+B.Happening+TEXT(" (no night happenings here)");
  // Asked as the night asks (not forced): its tier, the rhythm, the tutorial and its gates may say no; the reason goes to the log.
  if(Haps->Request(B.Happening,Why))return TEXT("happening starts: ")+B.Happening;
  UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story: happening %s refused: %s"),*B.Happening,*Why);
  return TEXT("happening refused: ")+B.Happening+TEXT(" (")+Why+TEXT(")");
 }
 case ES47Beat::Clock:
  if(B.AtLeast<8.)return TEXT("clock: no hour");
  ClockAt=FMath::Max(ClockAt,B.AtLeast);ClockWaited=0;
  return TEXT("clock to at least ")+S47StoryDirectorLocal::HourText(ClockAt);
 case ES47Beat::Objective:
  ObjectiveLine=Text.TrimStartAndEnd();
  return ObjectiveLine.IsEmpty()?FString(TEXT("objective cleared")):TEXT("objective: ")+ObjectiveLine;
 default:
  return TEXT("not a story beat");
 }
}

bool FS47StoryDirector::StartScript(const FString& Id,FString& Why)
{
 if(!FindScript(Id)){Why=TEXT("no story script ")+Id;return false;}
 if((Runner&&Runner->Running(Id))||Pending.ContainsByPredicate([&Id](const FPending& P){return P.Id==Id;})){Why=Id+TEXT(" is running already");return false;}
 Pending.Add(FPending{Id,0.,0});
 return true;
}

void FS47StoryDirector::QueueAct(int32 Index)
{
 if(!ActList.IsValidIndex(Index))return;
 for(const FString& Id:ActList[Index].Scripts)
 {
  FString Why;
  if(!StartScript(Id,Why))UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story act %s: script %s not started: %s"),*ActList[Index].Id,*Id,*Why);
 }
}

void FS47StoryDirector::StartPending()
{
 if(Pending.IsEmpty()||!Runner)return;
 // A start beat in what starts now waits for the next tick (the runner is never asked to start a run from inside its own step).
 const TArray<FPending> Due=MoveTemp(Pending);
 Pending.Reset();
 for(const FPending& P:Due)
 {
  const FS47EventScript* Script=FindScript(P.Id);
  if(!Script){UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story script %s is not in the store; left out"),*P.Id);continue;}
  if(!Runner->Resume(*Script,FS47EventCtx(),P.T,P.Next))UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story script %s could not start (no game, or too many runs)"),*P.Id);
 }
}

void FS47StoryDirector::StepClock(double Dt)
{
 if(ClockAt<0)return;
 double Clock=0;
 if(!M||!M->Freeform||!ClockNow(Clock)){ClockAt=-1;ClockWaited=0;return;}
 // Never under a panel or at the wheel: it waits a few seconds for them, then it is left out.
 if(M->ModalOpen()||(M->World&&M->World->bDriving))
 {
  ClockWaited+=FMath::Max(0.,Dt);
  if(ClockWaited>=ClockWaitMost)
  {
   UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story clock to %s left out: a panel open or driving for %.0f s"),*S47StoryDirectorLocal::HourText(ClockAt),ClockWaitMost);
   ClockAt=-1;ClockWaited=0;
  }
  return;
 }
 const double Push=ClockPushFor(Clock,ClockAt);
 if(Push>0&&M->Freeform->SpendTime(Push))UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story clock pushed %.0f min to %s"),Push/60.,*S47StoryDirectorLocal::HourText(ClockAt));
 ClockAt=-1;ClockWaited=0;
}

void FS47StoryDirector::SetHold(bool bOn)
{
 if(bOn==bHeld)return;
 bHeld=bOn;
 FS47ScriptPlayer::Hold(TEXT("story"),bOn);
 UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story %s Freeform's dice"),bOn?TEXT("holds"):TEXT("lets go of"));
}

void FS47StoryDirector::AddDevLines()
{
 using namespace S47StoryDirectorLocal;
 if(!M||!M->DevEvents)return;
 US47Main* Main=M;
 const auto AddOnce=[Main](const FString& Label,const FString& Note,TFunction<void()> Fn)
 {
  if(Main->DevEvents->Events().ContainsByPredicate([&Label](const FS47DevEvents::FEvent& Ev){return Ev.Group==DevGroup&&Ev.Label==Label;}))return;
  Main->DevEvents->AddStory(DevGroup,Label,Note,MoveTemp(Fn));
 };
 const auto Say=[Main](const FString& Line){if(Main->UI)Main->UI->Toast(TEXT("Developer event: ")+Line,5);};
 AddOnce(TEXT("Story director: state"),TEXT("the act, the flags, the objective and the story scripts running"),[Main,Say]
 {
  const FS47StoryDirector* D=Of(Main);
  Say(D?D->StateText():FString(TEXT("not a Story night")));
 });
 AddOnce(TEXT("Read the story scripts again"),TEXT("Content/Director/Story/beats; a new file gets its own line"),[Main,Say]
 {
  ReloadScripts();
  const TMap<FString,FString>& Bad=RefusedScripts();
  Say(FString::Printf(TEXT("%d story scripts, %d refused%s."),StoryScripts().Num(),Bad.Num(),Bad.Num()?*(TEXT(" (first: ")+Bad.CreateConstIterator().Value()+TEXT(")")):TEXT("")));
  if(FS47StoryDirector* D=Of(Main))D->AddDevLines();
 });
 for(const FS47EventScript& Script:StoryScripts())
 {
  const FString Id=Script.Id;
  AddOnce(TEXT("Run: ")+Id,Script.Name.IsEmpty()?FString(TEXT("this story script now (its flags and objective are kept)")):Script.Name,[Main,Say,Id]
  {
   FS47StoryDirector* D=Of(Main);
   FString Why=TEXT("not a Story night");
   const bool bOk=D&&D->StartScript(Id,Why);
   Say(bOk?TEXT("story script ")+Id+TEXT(" starts"):TEXT("not now: ")+Why);
  });
 }
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
 return FString::Printf(TEXT("act %s, %d of %d, flags [%s], objective \"%s\", %d story scripts running, %d to start"),*Now->Id,ActIndex+1,ActList.Num(),*FString::Join(FlagList(),TEXT(",")),*ObjectiveLine,RunningCount(),Pending.Num());
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
 // ---------- STORY-4 (step 4): the story's beats and conditions, its store, its lint, the clock, the objective, the save, the hold ----------
 {
  const FString NoGates;
  const auto Wrap=[](const FString& BeatJson,const FString& GatesJson){return FString::Printf(TEXT("{\"version\":1,\"id\":\"story_test\",%s\"beats\":[%s]}"),*GatesJson,*BeatJson);};
  const auto FirstBeat=[](const FString& Json){FS47EventScript Sc;FString W;return ReadScript(Json,Sc,W)&&Sc.Beats.Num()?Sc.Beats[0]:FS47Beat();};
  const FString FlagOn=TEXT("{\"at\":0,\"do\":\"flag\",\"flag\":\"a_flag\"}");
  const FString FlagOff=TEXT("{\"at\":0,\"do\":\"flag\",\"flag\":\"a_flag\",\"set\":false}");
  const FString ClockOne=TEXT("{\"at\":0,\"do\":\"clock\",\"at_least\":\"23:30\"}");
  const FString Goal=TEXT("{\"at\":0,\"do\":\"objective\",\"text\":\"Take the call at OPS.\"}");
  const FString GoalOff=TEXT("{\"at\":0,\"do\":\"objective\",\"text\":\"\"}");
  const TArray<FString> Samples={Wrap(FlagOn,NoGates),Wrap(FlagOff,NoGates),Wrap(TEXT("{\"at\":0,\"do\":\"start\",\"script\":\"other_script\"}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"start\",\"happening\":\"hitchhiker\"}"),NoGates),Wrap(ClockOne,NoGates),Wrap(Goal,NoGates),Wrap(GoalOff,NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"A line.\",\"if\":[{\"if\":\"story_flag\",\"flag\":\"a_flag\"}]}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"A line.\",\"wait\":[{\"if\":\"no_story_flag\",\"flag\":\"a_flag\"}],\"max\":5}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"A line.\"}"),TEXT("\"gates\":{\"if\":[{\"if\":\"story_flag\",\"flag\":\"a_flag\"}]},"))};
  bool bFreeRefuses=true,bStoryTakes=true;
  for(const FString& Json:Samples)
  {
   FS47EventScript Sample;FString SampleWhy;
   const bool bSampleRead=S47EventScript::Read(Json,Sample,SampleWhy);
   const TArray<FString> FreeErrors=bSampleRead?S47EventScript::Lint(Sample):TArray<FString>();
   bFreeRefuses&=bSampleRead&&FreeErrors.Num()>0&&FreeErrors[0].Contains(TEXT("story-only"));
   FS47EventScript StoryOne;FString StoryWhy;
   const bool bTaken=ReadScript(Json,StoryOne,StoryWhy);
   if(!bTaken)Lines.Add(TEXT("NOTE story lint refused a sample: ")+StoryWhy);
   bStoryTakes&=bTaken;
  }
  Check(bFreeRefuses,TEXT("STORY-4: Freeform's lint refuses story_flag, no_story_flag and the flag, start, clock and objective beats by name"));
  Check(bStoryTakes,TEXT("STORY-4: the story store's lint (S47EventScript::LintStory) lets them through"));
  bool bStoryRefuses=true;
  for(const FString& Wrong:{Wrap(TEXT("{\"at\":0,\"do\":\"flag\",\"flag\":\"Not A Flag\"}"),NoGates),Wrap(TEXT("{\"at\":0,\"do\":\"start\"}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"start\",\"script\":\"a_one\",\"happening\":\"hitchhiker\"}"),NoGates),Wrap(TEXT("{\"at\":0,\"do\":\"start\",\"script\":\"story_test\"}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"clock\",\"at_least\":\"25:00\"}"),NoGates),Wrap(TEXT("{\"at\":0,\"do\":\"clock\"}"),NoGates),
   Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"A line.\",\"if\":[{\"if\":\"story_flag\"}]}"),NoGates)})
  {
   FS47EventScript WrongOne;FString WrongWhy;
   bStoryRefuses&=!ReadScript(Wrong,WrongOne,WrongWhy);
  }
  Check(bStoryRefuses,TEXT("STORY-4: the story's lint refuses a bad flag id, a start with neither or both, a start of itself, a clock without HH:MM, a story_flag without flag"));
  { // the story's text by LintStory, Freeform's by LintText
   const FString Told=Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"The tape stops at 05:29:47, as it did in 1947.\"}"),NoGates);
   const FString Named=Wrap(TEXT("{\"at\":0,\"do\":\"toast\",\"text\":\"He shouts like Kinski.\"}"),NoGates);
   FS47EventScript TextOne;FString TextWhy;
   const bool bFreeNo=S47EventScript::Read(Told,TextOne,TextWhy)&&S47EventScript::Lint(TextOne).Num()>0;
   const bool bStoryYes=ReadScript(Told,TextOne,TextWhy);
   const bool bNamedNo=!ReadScript(Named,TextOne,TextWhy);
   Check(bFreeNo&&bStoryYes&&bNamedNo,TEXT("STORY-4: story text is checked by LintStory (the story's own words pass; a banned name does not); Freeform's lint still refuses them"));
  }
  FS47StoryDirector Dir(nullptr);
  Dir.BeginWith(ActsText,S47J::FObj());
  FS47ScriptCond Has;Has.If=TEXT("story_flag");Has.Arg=TEXT("a_flag");
  FS47ScriptCond HasNot;HasNot.If=TEXT("no_story_flag");HasNot.Arg=TEXT("a_flag");
  const bool bBefore=!Dir.FlagCond(Has)&&Dir.FlagCond(HasNot);
  Dir.Apply(FirstBeat(Wrap(FlagOn,NoGates)),FString());
  const bool bSetNow=Dir.HasFlag(TEXT("a_flag"))&&Dir.FlagCond(Has)&&!Dir.FlagCond(HasNot);
  Dir.Apply(FirstBeat(Wrap(FlagOff,NoGates)),FString());
  const bool bClearedNow=!Dir.HasFlag(TEXT("a_flag"))&&!Dir.FlagCond(Has)&&Dir.FlagCond(HasNot);
  Check(bSetNow&&bClearedNow,TEXT("STORY-4: the flag beat sets a story flag, and with set false clears it"));
  Check(bBefore&&bSetNow&&bClearedNow&&!CondHolds(nullptr,Has)&&CondHolds(nullptr,HasNot),TEXT("STORY-4: story_flag and no_story_flag follow the flags; without a Story night's director no story flag is set"));
  const FS47Beat GoalBeat=FirstBeat(Wrap(Goal,NoGates));
  Dir.Apply(GoalBeat,GoalBeat.Text);
  const bool bGoal=Dir.ObjectiveText()==TEXT("Take the call at OPS.");
  const FS47Beat GoalClear=FirstBeat(Wrap(GoalOff,NoGates));
  Dir.Apply(GoalClear,GoalClear.Text);
  Check(bGoal&&Dir.ObjectiveText().IsEmpty()&&ObjectiveOf(nullptr).IsEmpty(),TEXT("STORY-4: the objective beat sets the story's line and an empty one clears it; none without a director"));
  { // clock at_least: forward only, to that hour of tonight, never into the next night (pure)
   bool bForwardOnly=true;
   for(int32 Half=0;Half<96;++Half)for(int32 K=0;K<48;++K)
   {
    const double From=Half*1800.,Target=8.+K*.5;
    const double Push=ClockPushFor(From,Target);
    bForwardOnly&=Push>=0&&FS47FreeformSession::NightOf(From+Push)==FS47FreeformSession::NightOf(From);
    bForwardOnly&=Push>0?FMath::IsNearlyEqual(NightHourOf(From+Push),Target,1e-6):NightHourOf(From)>=Target;
   }
   double H2330=0,H2300=0;
   const bool bHours=ParseFrom(TEXT("23:30"),H2330)&&ParseFrom(TEXT("23:00"),H2300);
   const bool bCases=bHours&&FMath::IsNearlyEqual(ClockPushFor(82800.,H2330),1800.)&&ClockPushFor(Day2+3*3600.,H2300)==0&&ClockPushFor(82800.,H2300)==0
    &&ClockPushFor(82800.,5.)==0&&ClockPushFor(82800.,32.)==0&&FMath::IsNearlyEqual(FirstBeat(Wrap(ClockOne,NoGates)).AtLeast,23.5);
   Check(bForwardOnly&&bCases,TEXT("STORY-4: clock at_least only pushes the clock forward, to that hour of tonight, never back and never into the next night"));
  }
  FS47StoryDirector Idle(nullptr);
  const bool bIdle=!Idle.Busy()&&!Holds(nullptr)&&!Of(nullptr);
  Dir.Apply(FirstBeat(Wrap(ClockOne,NoGates)),FString());
  Check(bIdle&&Dir.Busy()&&!Holds(nullptr),TEXT("STORY-4: Holds is false without a director; a waiting story beat makes the director busy"));
  { // the save: objective and the scripts under way, v stays 1; absent fields read as none
   FS47StoryDirector Saver(nullptr);
   Saver.BeginWith(ActsText,S47J::Read(TEXT("{\"v\":1,\"act\":\"night\",\"flags\":[\"a_flag\"],\"objective\":\"Take the call at OPS.\",\"scripts\":[{\"id\":\"evening_handover\",\"t\":2.5,\"next\":1},{\"id\":\"Bad Id\",\"t\":1,\"next\":0}]}")));
   const S47J::FObj Kept=S47J::Read(S47J::Write(Saver.Save()));
   const TArray<TSharedPtr<FJsonValue>> Under=S47J::Arr(Kept,TEXT("scripts"));
   const S47J::FObj* Entry=nullptr;
   const bool bRun=Under.Num()==1&&Under[0].IsValid()&&Under[0]->TryGetObject(Entry)&&Entry&&S47J::Str(*Entry,TEXT("id"))==TEXT("evening_handover")
    &&FMath::IsNearlyEqual(S47J::Num(*Entry,TEXT("t")),2.5)&&S47J::Int(*Entry,TEXT("next"))==1;
   FS47StoryDirector Older(nullptr);
   Older.BeginWith(ActsText,S47J::Read(TEXT("{\"v\":1,\"act\":\"night\",\"flags\":[]}")));
   const S47J::FObj OlderSaved=Older.Save();
   Check(bRun&&S47J::Int(Kept,TEXT("v"))==1&&S47J::Str(Kept,TEXT("objective"))==TEXT("Take the call at OPS.")&&Saver.ObjectiveText()==TEXT("Take the call at OPS.")&&Saver.CurrentIndex()==1
    &&Older.ObjectiveText().IsEmpty()&&!Older.Busy()&&!S47J::Has(OlderSaved,TEXT("scripts"))&&!S47J::Has(OlderSaved,TEXT("objective")),
    TEXT("STORY-4: the save keeps the objective and the story scripts under way (v stays 1); a save without them has none"));
  }
  { // acts list the scripts they start
   TArray<FAct> WithScripts,WrongScripts;FString ActWhy;
   const FString ActsWith=TEXT("{\"v\":1,\"acts\":[{\"id\":\"evening\",\"title\":\"Evening\",\"from\":\"18:00\",\"scripts\":[\"evening_handover\"]},{\"id\":\"night\",\"title\":\"Night\",\"from\":\"00:00\"}]}");
   const bool bActs=ParseActs(ActsWith,WithScripts,ActWhy)&&WithScripts[0].Scripts.Num()==1&&WithScripts[0].Scripts[0]==TEXT("evening_handover")&&WithScripts[1].Scripts.IsEmpty();
   const bool bActRefuses=!ParseActs(TEXT("{\"v\":1,\"acts\":[{\"id\":\"evening\",\"title\":\"Evening\",\"from\":\"18:00\",\"scripts\":[\"Bad Id\"]}]}"),WrongScripts,ActWhy);
   Check(bActs&&bActRefuses,TEXT("STORY-4: an act lists the story scripts it starts; a bad id refuses the acts"));
   ReloadScripts();
   for(const TPair<FString,FString>& Refused:RefusedScripts())Lines.Add(FString::Printf(TEXT("NOTE story script %s refused: %s"),*Refused.Key,*Refused.Value));
   const FS47EventScript* Handover=FindScript(TEXT("evening_handover"));
   if(!Handover)Lines.Add(TEXT("NOTE story director: no Content/Director/Story/beats/evening_handover.json beside this build"));
   else
   {
    const bool bShape=Handover->Beats.ContainsByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Flag&&Bt.Flag==TEXT("handover_told")&&Bt.bSet;})
     &&Handover->Beats.ContainsByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Objective&&Bt.Text==TEXT("Take the evening handover call at OPS.");})
     &&Handover->Beats.ContainsByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Toast&&!Bt.Text.IsEmpty();});
    FS47StoryDirector Fresh(nullptr);
    Fresh.BeginWith(ActsWith,S47J::FObj());
    FS47StoryDirector Later(nullptr);
    Later.BeginWith(ActsWith,S47J::Read(TEXT("{\"v\":1,\"act\":\"evening\"}")));
    Check(bShape&&S47EventScript::Lint(*Handover).Num()>0&&Fresh.Busy()&&!Later.Busy(),TEXT("STORY-4: evening_handover is in the story store, Freeform's lint refuses it, and the evening act starts it (once: not again from a save)"));
   }
   FString StoryDir=ScriptsDir();
   FPaths::NormalizeDirectoryName(StoryDir);FPaths::CollapseRelativeDirectories(StoryDir);
   Check(!S47EventScript::ReadDirs().Contains(StoryDir)&&!S47EventScript::Find(TEXT("evening_handover")),TEXT("STORY-4: Freeform's event folders do not read the story's scripts"));
  }
 }
 return OK;
}
