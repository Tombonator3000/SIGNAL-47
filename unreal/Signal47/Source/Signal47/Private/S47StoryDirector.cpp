#include "S47StoryDirector.h"
#include "S47StoryScene.h"
#include "S47StoryDrive.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Script.h"
#include "S47DevEvents.h"
#include "S47Flow.h"
#include "S47Happenings.h"
#include "S47ScriptPlayer.h"
#include "S47UI.h"
#include "S47World.h"
#include "S47RoadShape.h"
#include "S47DeputyRound.h"
#include "S47Hide.h"
#include "S47Attention.h"
#include "S47Taken.h"
#include "S47SkyEventSpec.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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

FS47StoryDirector::FS47StoryDirector(US47Main* Main):M(Main),Runner(MakeUnique<FS47EventBeats>(Main)),Scene(MakeUnique<FS47StoryScene>(Main)),Wagon(MakeUnique<FS47StoryDrive>(Main))
{
 Scene->OnFlag=[this](const FString& F){SetFlag(F);};
}

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
 ActIndex=0;Flags.Reset();ObjectiveLine.Reset();Pending.Reset();HoleFrom=-1;bEndDue=false;bEnded=false;
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
 // Step 6 (absent in older saves: none): the star hole's start, once it has opened.
 {
  const double Hole=S47J::Num(Saved,TEXT("holeFrom"),-1);
  if(FMath::IsFinite(Hole)&&Hole>=0&&Hole<86400.+STAR_HOLE.After)HoleFrom=Hole;
 }
 // Step 6, the morning act (absent in older saves: not due): the end beat had come and the end was not played yet.
 bEndDue=S47J::Bool(Saved,TEXT("ending"),false)&&!Flags.Contains(EndFlag);
 // Step 5 (absent in older saves: no scene): the scene under way, taken up again.
 const S47J::FObj* SceneSaved=nullptr;
 if(Scene&&Saved->TryGetObjectField(TEXT("scene"),SceneSaved)&&SceneSaved)Scene->Load(*SceneSaved);
 // STORY-DRIVE-OWN-CAR-20261113 (absent in older saves: no wagon, the truck as it was saved): the wagon on the way, or parked at SARO.
 const S47J::FObj* CarSaved=nullptr;
 if(Wagon&&Saved->TryGetObjectField(TEXT("car"),CarSaved)&&CarSaved)Wagon->Load(*CarSaved);
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
 if(Scene)Scene->Stop();
 if(Wagon)Wagon->Stop(); // STORY-DRIVE-OWN-CAR-20261113
 LastEvent.Reset();LastGround.Reset();LastPatrol.Reset();HoleFrom=-1;
 LastVisit.Reset();LastTaken=-1;bEndDue=false;bEnded=false; // step 6, the morning act
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
 // Step 6: Freeform's world told to the story. The power fault (feeder_fault, or any other) as story flags, its rise and its repair;
 // the pack's ctx_* flags from the same (S47StoryScene::SetContext). power_fault is set while it lasts, so a save keeps the state.
 if(M&&M->Freeform)
 {
  const bool bFault=M->Freeform->PowerFault();
  if(bFault&&!HasFlag(TEXT("power_fault")))SetFlag(TEXT("power_fault"));
  if(!bFault&&HasFlag(TEXT("power_fault"))){ClearFlag(TEXT("power_fault"));SetFlag(TEXT("power_restored"));}
  if(Scene)Scene->SetContext({{TEXT("ctx_power_available"),!bFault},{TEXT("ctx_trafo_service_ok"),HasFlag(TEXT("power_restored"))},{TEXT("ctx_room_quiet"),!M->ModalOpen()}});
 }
 // Step 6, the night act: Freeform's own events told to the story as they end (EndedFlagOf): a Director event or a night happening on
 // the ground that is over sets ended_<id>, the county car gone by sets ended_deputy_round; hid once the player has hidden (FS47Hide).
 if(M&&M->Freeform)
 {
  const FS47FreeformSession& Ff=*M->Freeform;
  const FString EventNow=Ff.Director?Ff.Director->ActiveEvent():FString();
  const FString GroundNow=Ff.Happenings?Ff.Happenings->Ground():FString();
  const FString PatrolNow=Ff.Deputy&&Ff.Deputy->Driving()?FString(DeputyRoundId):FString();
  for(const FString& Ended:{EndedFlagOf(LastEvent,EventNow),EndedFlagOf(LastGround,GroundNow),EndedFlagOf(LastPatrol,PatrolNow)})
  {
   if(!Ended.IsEmpty()&&SetFlag(Ended)){UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story flag %s"),*Ended);}
  }
  LastEvent=EventNow;LastGround=GroundNow;LastPatrol=PatrolNow;
  if(Ff.Hide&&Ff.Hide->Hidden()&&!HasFlag(TEXT("hid")))SetFlag(TEXT("hid"));
  // The morning act (SC11): Freeform's visit at SARO told as it ends (ended_attention_visit), and the player taken this Story night.
  const FString VisitNow=Ff.Attention&&Ff.Attention->Visiting()?FString(AttentionVisitId):FString();
  const FString VisitEnded=EndedFlagOf(LastVisit,VisitNow);
  if(!VisitEnded.IsEmpty()&&SetFlag(VisitEnded)){UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story flag %s"),*VisitEnded);}
  LastVisit=VisitNow;
  const int32 TakenNow=Ff.TimesTaken();
  if(LastTaken>=0&&TakenNow>LastTaken&&!HasFlag(TakenFlag)){SetFlag(TakenFlag);UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story flag %s"),TakenFlag);}
  LastTaken=TakenNow;
 }
 // STORY-DRIVE-OWN-CAR-20261113: the wagon (its pose, its parking at SARO) and its radio's bed, before the radio scene's line.
 if(Wagon)Wagon->Tick(Dt,Scene&&Scene->CarRadio());
 if(Scene)Scene->Tick(Dt);
 StepClock(Dt);
 SetHold(M!=nullptr&&Busy());
 // Step 6 (SC13): the end beat has come. Not under a panel or while the player is being taken; then the end, once. The director is
 // stopped by the end itself (US47Flow::StoryNightEnd), so nothing of it runs after this.
 if(bEndDue&&!bEnded&&M&&M->Flow)
 {
  const bool bTaking=M->Freeform&&M->Freeform->Taken&&M->Freeform->Taken->Busy();
  if(EndMay(M->ModalOpen(),bTaking))
  {
   bEnded=true;bEndDue=false;
   SetFlag(EndFlag);
   UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story: the end (%s)"),*StateText());
   M->Flow->StoryNightEnd();
  }
 }
}

S47J::FObj FS47StoryDirector::Save() const
{
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("v"),1);
 if(const FAct* Now=CurrentAct())S47J::Set(O,TEXT("act"),Now->Id);
 S47J::Set(O,TEXT("flags"),FlagList());
 // Step 4: the objective, and the story scripts under way (and those about to start) with how far they had come.
 if(!ObjectiveLine.IsEmpty())S47J::Set(O,TEXT("objective"),ObjectiveLine);
 if(HoleFrom>=0)S47J::Set(O,TEXT("holeFrom"),HoleFrom); // step 6: the star hole
 if(bEndDue)S47J::Set(O,TEXT("ending"),true); // step 6, the morning act: the end beat has come
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
 // Step 5: the scene under way (the pack's own save: its scene, line, pending reply and flags).
 if(Scene&&Scene->Busy())O->SetObjectField(TEXT("scene"),Scene->Save());
 if(Wagon)if(const S47J::FObj Car=Wagon->Save();Car.IsValid())O->SetObjectField(TEXT("car"),Car); // STORY-DRIVE-OWN-CAR-20261113
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

bool FS47StoryDirector::UsePhone(const US47Main* Main)
{
 const FS47StoryDirector* D=Of(Main);
 return D&&D->Scene&&D->Scene->UsePhone();
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

double FS47StoryDirector::HoleFromAt(double Clock)
{
 const double Sec=FMath::IsFinite(Clock)?FMath::Fmod(FMath::Fmod(Clock,86400.)+86400.,86400.):0.;
 return Sec+STAR_HOLE.After;
}

TOptional<double> FS47StoryDirector::StarHoleFrom(const US47Main* Main)
{
 const FS47StoryDirector* D=Of(Main);
 return D&&D->HoleFrom>=0?TOptional<double>(D->HoleFrom):TOptional<double>();
}

FString FS47StoryDirector::EndedFlagOf(const FString& Was,const FString& Now)
{
 if(Was.IsEmpty()||Was==Now)return FString();
 const FString Flag=TEXT("ended_")+Was;
 return ValidId(Flag)?Flag:FString();
}

double FS47StoryDirector::ClockPushFor(double Clock,double AtLeast)
{
 if(!FMath::IsFinite(Clock)||!FMath::IsFinite(AtLeast)||AtLeast<8.||AtLeast>=32.)return 0;
 const double Hour=NightHourOf(Clock);
 return AtLeast>Hour?(AtLeast-Hour)*3600.:0.;
}

bool FS47StoryDirector::Busy() const
{
 return Pending.Num()>0||ClockAt>=0||bEndDue||(Runner&&Runner->Live()>0)||(Scene&&Scene->Busy());
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
  if(!B.Event.IsEmpty())
  { // step 6: one of Freeform's Director events, started for the story (as F10's Force, without its developer gate)
   FS47FreeformDirector* Dir=M&&M->Freeform?M->Freeform->Director.Get():nullptr;
   if(!Dir)return TEXT("event not started: ")+B.Event+TEXT(" (no Freeform director)");
   return Dir->StartForStory(B.Event,Why)?TEXT("event starts: ")+B.Event:TEXT("event not started: ")+B.Event+TEXT(" (")+Why+TEXT(")");
  }
  if(B.Happening==DeputyRoundId)
  { // step 6, the night act (SC03): Freeform's county patrol sent tonight (FS47DeputyRound), the story's only call of that name
   FS47DeputyRound* Patrol=M&&M->Freeform?M->Freeform->Deputy.Get():nullptr;
   if(!Patrol)return TEXT("county car not sent (no patrol here)");
   return Patrol->ForStory(Why)?FString(TEXT("county car sent")):TEXT("county car not sent (")+Why+TEXT(")");
  }
  if(B.Happening==AttentionVisitId)
  { // step 6, the morning act (SC11): Freeform's visit at SARO tonight. When it cannot begin its end is told at once, so the story goes on.
   FS47Attention* Att=M&&M->Freeform?M->Freeform->Attention.Get():nullptr;
   if(Att&&Att->StartForStory(Why))return TEXT("the visit at SARO begins");
   if(!Att)Why=TEXT("no attention here");
   SetFlag(TEXT("ended_")+FString(AttentionVisitId));
   UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story: the visit at SARO did not begin: %s"),*Why);
   return TEXT("the visit at SARO did not begin (")+Why+TEXT("); its end is told at once");
  }
  FS47Happenings* Haps=M&&M->Freeform?M->Freeform->Happenings.Get():nullptr;
  if(!Haps)return TEXT("happening not asked for: ")+B.Happening+TEXT(" (no night happenings here)");
  if(B.bForce) // step 6, the night act: started for the story, past its day and the rhythm's draw (a first night has neither)
  {
   if(Haps->StartForStory(B.Happening,Why))return TEXT("happening starts for the story: ")+B.Happening;
   UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story: happening %s not started: %s"),*B.Happening,*Why);
   return TEXT("happening not started: ")+B.Happening+TEXT(" (")+Why+TEXT(")");
  }
  // Asked as the night asks (not forced): its tier, the rhythm, the tutorial and its gates may say no; the reason goes to the log.
  if(Haps->Request(B.Happening,Why))return TEXT("happening starts: ")+B.Happening;
  UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story: happening %s refused: %s"),*B.Happening,*Why);
  return TEXT("happening refused: ")+B.Happening+TEXT(" (")+Why+TEXT(")");
 }
 case ES47Beat::Clock:
  if(B.AtLeast<8.)return TEXT("clock: no hour");
  ClockAt=FMath::Max(ClockAt,B.AtLeast);ClockWaited=0;
  return TEXT("clock to at least ")+S47StoryDirectorLocal::HourText(ClockAt);
 case ES47Beat::Scene:
 {
  // Step 5: one scene of the EveningShift pack on Freeform's telephone, walkie or radio (S47StoryScene.h).
  FString Why;
  if(!Scene)return TEXT("scene left out: no host");
  return Scene->Begin(B.Name,Why)?TEXT("scene starts: ")+B.Name:TEXT("scene not started: ")+B.Name+TEXT(" (")+Why+TEXT(")");
 }
 case ES47Beat::Objective:
  ObjectiveLine=Text.TrimStartAndEnd();
  return ObjectiveLine.IsEmpty()?FString(TEXT("objective cleared")):TEXT("objective: ")+ObjectiveLine;
 case ES47Beat::Telex:
 { // step 6, the morning act (SC11): a sheet on Freeform's own telex in the control room, printed and torn off there as any other
  FS47FreeformSession* Ff=M?M->Freeform.Get():nullptr;
  if(!Ff)return TEXT("telex left out: no Freeform session");
  return Ff->QueueTelex(B.Key,B.Name,B.Kind,Text)?TEXT("telex: ")+B.Key+TEXT(" prints"):TEXT("telex left out: ")+B.Key+TEXT(" (on the machine already, or the machine is full)");
 }
 case ES47Beat::End:
  // Step 6 (SC13): due now; Tick plays it once the player is out of a panel and not being taken.
  if(bEnded||bEndDue)return TEXT("the end is under way already");
  bEndDue=true;
  return TEXT("the story ends");
 default:
  return TEXT("not a story beat");
 }
}

FS47StoryDirector::FDrive FS47StoryDirector::DriveStart()
{
 FDrive D;
 D.Leg=TEXT("road");D.To=TEXT("saro");
 D.At=FVector(8001.8,0,S47RoadShape::TOWN().Z1+35);D.Heading=0;
 return D;
}

bool FS47StoryDirector::BeginDrive(US47Main* Main)
{
 if(FParse::Param(FCommandLine::Get(),TEXT("S47StoryAtSaro"))){UE_LOG(LogS47StoryDirector,Display,TEXT("S47: story night starts at SARO (-S47StoryAtSaro)"));return false;}
 if(!Main||!IsValid(Main->World)||Main->World->bBusy||Main->World->bDriving){UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story night: the drive to SARO could not start; the night starts at SARO"));return false;}
 // STORY-DRIVE-OWN-CAR-20261113 (Tom 13 Nov): the player's own station wagon, as the old Story's drive.
 if(Main->StoryDirector&&Main->StoryDirector->Active()&&Main->StoryDirector->Wagon&&Main->StoryDirector->Wagon->BeginNew())return true;
 const FDrive D=DriveStart();
 FS47TripOpts O;
 O.Leg=D.Leg;O.AtPos=D.At;O.AtHeading=D.Heading;
 O.Hint=TEXT("SARO: leave the truck on its pad by the station and go in to OPS.");
 Main->World->StartTrip(D.Leg,D.To,O);
 UE_LOG(LogS47StoryDirector,Warning,TEXT("S47: story night: the wagon could not start; in the truck on the %s leg at %.1f %.1f, heading for %s"),*D.Leg,D.At.X,D.At.Z,*D.To);
 return true;
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
 // Step 6: the star hole opens with its flag, once (its start kept through a save).
 if(Flag==StarHoleFlag&&HoleFrom<0){double Now=0;if(ClockNow(Now))HoleFrom=HoleFromAt(Now);}
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
 { // STORY-DRIVE-20261112: a new Story night begins on the road to SARO (STORY-DRIVE-OWN-CAR-20261113: in the player's own wagon)
  const FDrive D=DriveStart();
  Check(D.Leg==TEXT("road")&&D.To==TEXT("saro")&&FMath::IsNearlyEqual(D.At.X,8001.8)&&FMath::IsNearlyEqual(D.At.Z,S47RoadShape::TOWN().Z1+35)&&D.Heading==0,
   TEXT("STORY-8: a new Story night starts on the road leg at the old Story's drive start (US47World::BeginCommute's), heading north for SARO"));
  { // STORY-DRIVE-OWN-CAR-20261113: the drive's "car" is in the director's save only while there is one; an older save reads as none.
   FS47StoryDirector Old(nullptr);
   Old.BeginWith(TEXT("{\"v\":1,\"acts\":[{\"id\":\"evening\",\"title\":\"Evening\",\"from\":\"17:00\"}]}"),S47J::Read(TEXT("{\"v\":1,\"act\":\"evening\",\"flags\":[\"drive_begun\"]}")));
   const S47J::FObj Kept=Old.Save();
   Check(Old.Active()&&Old.HasFlag(TEXT("drive_begun"))&&Old.DriveHost()&&Old.DriveHost()->GetPhase()==FS47StoryDrive::EPhase::None&&Kept.IsValid()&&!Kept->HasField(TEXT("car")),
    TEXT("STORY-DRIVE-OWN-CAR: a Story save from before the wagon loads as before (no car in it, none written back)"));
  }
  const FS47EventScript* Drive=FindScript(TEXT("evening_drive"));
  const FS47EventScript* Diner=FindScript(TEXT("evening_diner"));
  if(!Drive||!Diner)Lines.Add(TEXT("NOTE story director: no evening_drive.json or evening_diner.json in Content/Director/Story/beats beside this build"));
  else
  {
   const auto Has=[](const FS47EventScript& Sc,TFunction<bool(const FS47Beat&)> Fn){return Sc.Beats.ContainsByPredicate(Fn);};
   const auto WaitsOn=[](const FS47Beat& Bt,const TCHAR* If,const TCHAR* Arg){return Bt.Wait.ContainsByPredicate([If,Arg](const FS47ScriptCond& C){return C.If==If&&(!Arg||C.Arg==Arg);});};
   const int32 Arrive=Drive->Beats.IndexOfByPredicate([&WaitsOn](const FS47Beat& Bt){return Bt.Do==ES47Beat::Flag&&Bt.Flag==TEXT("arrived_saro")&&WaitsOn(Bt,TEXT("area"),TEXT("saro"))&&WaitsOn(Bt,TEXT("on_foot"),nullptr);});
   const int32 Begun=Drive->Beats.IndexOfByPredicate([&WaitsOn](const FS47Beat& Bt){return Bt.Do==ES47Beat::Flag&&Bt.Flag==TEXT("drive_begun")&&WaitsOn(Bt,TEXT("in_truck"),nullptr);});
   const int32 Hands=Drive->Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Script==TEXT("evening_handover");});
   const bool bRadio=Has(*Drive,[](const FS47Beat& Bt){return Bt.Do==ES47Beat::Scene&&Bt.Name==TEXT("I01_DRIVE");});
   Check(S47EventScript::LintStory(*Drive).IsEmpty()&&S47EventScript::LintStory(*Diner).IsEmpty()&&S47EventScript::Lint(*Drive).Num()>0&&S47EventScript::Lint(*Diner).Num()>0,
    TEXT("STORY-8: evening_drive and evening_diner pass the story's lint; Freeform's lint refuses them"));
   Check(bRadio&&Begun>=0&&Arrive>Begun&&Hands>Arrive,TEXT("STORY-8: evening_drive waits for the truck, plays I01_DRIVE on its radio, waits for the player on foot at SARO, then starts evening_handover"));
   Check(Has(*Diner,[](const FS47Beat& Bt){return Bt.Do==ES47Beat::Scene&&Bt.Name==TEXT("I02_DINER_HELLO");})&&Has(*Diner,[](const FS47Beat& Bt){return Bt.Do==ES47Beat::Scene&&Bt.Name==TEXT("I10_BACK_ON_ROAD");}),
    TEXT("STORY-8: evening_diner greets a stop at the Mesa Diner (I02) and plays I10 back on the road"));
   // The acts beside the build start the drive first, and a fresh director has it under way; a save in the evening act does not again.
   FString Text;
   TArray<FAct> Parsed;FString Why;
   const bool bActs=FFileHelper::LoadFileToString(Text,*ActsPath())&&ParseActs(Text,Parsed,Why)&&Parsed.Num()>0&&Parsed[0].Scripts.Num()>0&&Parsed[0].Scripts[0]==TEXT("evening_drive")
    &&Parsed[0].Scripts.Contains(TEXT("evening_diner"))&&!Parsed[0].Scripts.Contains(TEXT("evening_handover"));
   FS47StoryDirector Fresh(nullptr);
   Fresh.BeginWith(Text,S47J::FObj());
   const bool bQueued=Fresh.Pending.ContainsByPredicate([](const FPending& P){return P.Id==TEXT("evening_drive");})&&!Fresh.Pending.ContainsByPredicate([](const FPending& P){return P.Id==TEXT("evening_handover");});
   Check(bActs&&bQueued,TEXT("STORY-8: the evening act starts evening_drive and evening_diner (the handover comes from the drive, not the act)"));
  }
 }
 { // Step 5: the scene beat
  const FS47EventScript* Handover=FindScript(TEXT("evening_handover"));
  const bool bScenes=Handover&&Handover->Beats.ContainsByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Scene&&Bt.Name==TEXT("D01_HANDOVER");});
  FS47EventScript Bad;Bad.Id=TEXT("story_scene_probe");Bad.Name=TEXT("probe");
  FS47Beat Sc;Sc.Do=ES47Beat::Scene;Sc.Name=TEXT("D01_HANDOVER");Bad.Beats.Add(Sc);
  FS47EventScript Unknown=Bad;Unknown.Beats[0].Name=TEXT("NO_SUCH_SCENE");
  const bool bLint=S47EventScript::Lint(Bad).Num()>0&&S47EventScript::LintStory(Unknown).Num()>0;
  Check(bScenes&&bLint,TEXT("STORY-5: evening_handover plays the handover as a scene; Freeform's lint refuses a scene beat, the story's refuses an unknown scene"));
  OK&=FS47StoryScene::SelfTest(Lines);
  OK&=FS47StoryDrive::SelfTest(Lines); // STORY-DRIVE-OWN-CAR-20261113: the wagon, its radio's beds, its save
  // Step 6: a start beat's event, and the evening's second script
  FS47EventScript Ev;Ev.Id=TEXT("story_event_probe");Ev.Name=TEXT("probe");
  FS47Beat St;St.Do=ES47Beat::Start;St.Event=TEXT("feeder_fault");Ev.Beats.Add(St);
  FS47EventScript Two=Ev;Two.Beats[0].Script=TEXT("evening_shift");
  const FS47EventScript* Shift=FindScript(TEXT("evening_shift"));
  const bool bShift=Shift&&Shift->Beats.ContainsByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Event==TEXT("feeder_fault");})&&S47EventScript::LintStory(*Shift).IsEmpty();
  Check(S47EventScript::LintStory(Ev).IsEmpty()&&S47EventScript::LintStory(Two).Num()>0&&S47EventScript::Lint(Ev).Num()>0&&bShift,TEXT("STORY-6: start may name a Freeform Director event (one of script, happening, event); Freeform's lint refuses it; evening_shift starts feeder_fault"));
 }
 { // STORY steg 6, the night act: SC02, SC03, SC05, SC08 and SC10 on Freeform's own events, chained by their last beats
  static const TCHAR* const NightChain[]={TEXT("night_hunt"),TEXT("night_deputy"),TEXT("night_residual"),TEXT("night_array"),TEXT("night_room6"),TEXT("night_room6_door")};
  TArray<const FS47EventScript*> Chain;
  for(const TCHAR* ChainId:NightChain)Chain.Add(FindScript(ChainId));
  if(Chain.Contains(nullptr))Lines.Add(TEXT("NOTE story director: a night_*.json of the night act is missing in Content/Director/Story/beats beside this build"));
  else
  {
   bool bLint=true;
   for(const FS47EventScript* Sc:Chain)bLint&=S47EventScript::LintStory(*Sc).IsEmpty()&&S47EventScript::Lint(*Sc).Num()>0;
   Check(bLint,TEXT("STORY-9: the night act's scripts (night_hunt, night_deputy, night_residual, night_array, night_room6) pass the story's lint; Freeform's lint refuses each"));
   // Each starts the next as its last beat, and no other story script: one after another, never twice.
   bool bOrder=true;
   for(int32 I=0;I<Chain.Num();++I)
   {
    const FS47EventScript& Link=*Chain[I];
    TArray<FString> Starts;
    for(const FS47Beat& Bt:Link.Beats)if(Bt.Do==ES47Beat::Start&&!Bt.Script.IsEmpty())Starts.Add(Bt.Script);
    if(I+1==Chain.Num())bOrder&=Starts.IsEmpty();
    else bOrder&=Starts.Num()==1&&Starts[0]==NightChain[I+1]&&Link.Beats.Num()>0&&Link.Beats.Last().Do==ES47Beat::Start&&Link.Beats.Last().Script==NightChain[I+1];
   }
   FString NightText,NightWhy;
   TArray<FAct> NightActs;
   const bool bAct=FFileHelper::LoadFileToString(NightText,*ActsPath())&&ParseActs(NightText,NightActs,NightWhy)
    &&NightActs.ContainsByPredicate([](const FAct& A){return A.Id==TEXT("night")&&A.Scripts.Num()==1&&A.Scripts[0]==TEXT("night_hunt");});
   Check(bOrder&&bAct,TEXT("STORY-10: the night act starts night_hunt only; the hunt, the deputy, the residual, the array and room 6 follow one another by their last beats"));
   // The full encounters wait for their tier (Tom: full encounters after 02:00): a clock beat to at least that hour comes first.
   const auto ClockFirst=[](const FS47EventScript& Sc,const TCHAR* EventName,double Least)
   {
    const int32 StartIdx=Sc.Beats.IndexOfByPredicate([EventName](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Event==EventName;});
    const int32 ClockIdx=Sc.Beats.IndexOfByPredicate([Least](const FS47Beat& Bt){return Bt.Do==ES47Beat::Clock&&Bt.AtLeast>=Least;});
    return StartIdx>=0&&ClockIdx>=0&&ClockIdx<StartIdx;
   };
   Check(ClockFirst(*Chain[0],TEXT("service_presence"),26.)&&ClockFirst(*Chain[3],TEXT("array_presence"),28.),
    TEXT("STORY-11: the full encounters wait for their tier: the hunt (service_presence) for 02:00, the array (array_presence) for 04:00"));
   // Tom (11 Nov): never change the star sky. Nothing of the night act starts anything in the sky (SC05's star hole is left out).
   static const TCHAR* const SkyIds[]={TEXT("flyover"),TEXT("close_pass"),TEXT("sky_formation"),TEXT("sky_high"),TEXT("landing"),TEXT("response_lights"),TEXT("dusk_orb"),TEXT("highway_ufo"),TEXT("farm_lights"),TEXT("town_lights")};
   bool bNoSky=true;
   for(const FS47EventScript* Sc:Chain)for(const FS47Beat& Bt:Sc->Beats)for(const TCHAR* SkyId:SkyIds)bNoSky&=!(Bt.Do==ES47Beat::Start&&(Bt.Event==SkyId||Bt.Happening==SkyId));
   // Tom (12 Nov): the star hole is Story's and stays (SC05): night_residual opens it by its flag, after the dishes have been heard.
   const FS47EventScript& Residual=*Chain[2];
   const int32 HoleIdx=Residual.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Flag&&Bt.bSet&&Bt.Flag==StarHoleFlag;});
   const int32 CreakIdx=Residual.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Event==TEXT("antenna_creak");});
   Check(bNoSky&&CreakIdx>=0&&HoleIdx>CreakIdx,TEXT("STORY-12: no night script starts anything in the sky (no other change to the star sky); night_residual opens the star hole after the dishes are heard"));
   // A wait on ended_<id> follows that id's own start in the same script, and the flag is cleared before it (a stale end never passes).
   bool bEnds=true;int32 Waits=0;
   for(const FS47EventScript* Sc:Chain)
   {
    for(int32 K=0;K<Sc->Beats.Num();++K)for(const FS47ScriptCond& C:Sc->Beats[K].Wait)
    {
     if(C.If!=TEXT("story_flag")||!C.Arg.StartsWith(TEXT("ended_")))continue;
     ++Waits;
     const FString EndedId=C.Arg.Mid(6);
     bool bStarted=false,bCleared=false;
     for(int32 J=0;J<K;++J)
     {
      const FS47Beat& Prev=Sc->Beats[J];
      bStarted|=Prev.Do==ES47Beat::Start&&(Prev.Event==EndedId||Prev.Happening==EndedId);
      bCleared|=Prev.Do==ES47Beat::Flag&&!Prev.bSet&&Prev.Flag==C.Arg;
     }
     bEnds&=bStarted&&bCleared;
    }
   }
   Check(bEnds&&Waits>=6,TEXT("STORY-15: each wait on a Freeform event's end (ended_<id>) follows that event's start and a clearing of the flag in the same script"));
  }
  // The start beat's force, and the ended_ flags.
  FS47EventScript Forced;Forced.Id=TEXT("story_force_probe");Forced.Name=TEXT("probe");
  FS47Beat Fb;Fb.Do=ES47Beat::Start;Fb.Happening=TEXT("room6_tv");Fb.bForce=true;Forced.Beats.Add(Fb);
  FS47EventScript ForcedScript=Forced;ForcedScript.Beats[0].Happening.Reset();ForcedScript.Beats[0].Script=TEXT("night_hunt");
  FS47EventScript ReadForce;FString ForceWhy;
  const bool bReadForce=S47EventScript::Read(TEXT("{\"version\":1,\"id\":\"story_force_read\",\"name\":\"probe\",\"beats\":[{\"at\":1,\"do\":\"start\",\"happening\":\"room6_tv\",\"force\":true}]}"),ReadForce,ForceWhy)
   &&ReadForce.Beats.Num()==1&&ReadForce.Beats[0].bForce;
  Check(bReadForce&&S47EventScript::LintStory(Forced).IsEmpty()&&S47EventScript::LintStory(ForcedScript).Num()>0&&S47EventScript::Lint(Forced).Num()>0,
   TEXT("STORY-13: start's force is read; the story's lint takes it with a happening and refuses it with a script; Freeform's lint refuses it"));
  Check(EndedFlagOf(TEXT("service_presence"),FString())==TEXT("ended_service_presence")&&EndedFlagOf(TEXT("room6_tv"),TEXT("taken"))==TEXT("ended_room6_tv")
   &&EndedFlagOf(TEXT("wrong_source"),TEXT("wrong_source")).IsEmpty()&&EndedFlagOf(FString(),TEXT("array_presence")).IsEmpty()
   &&EndedFlagOf(FString(DeputyRoundId),FString())==TEXT("ended_deputy_round")&&EndedFlagOf(TEXT("Not An Id"),FString()).IsEmpty(),
   TEXT("STORY-14: a Freeform event's end is the story flag ended_<id> (the Director's event, the happening on the ground, the county car); nothing when nothing ended"));
  // The star hole's start: set once with its flag at the clock then, kept through a save, and none without a Story night's director.
  FS47StoryDirector HoleDir(nullptr);
  HoleDir.BeginWith(ActsText,S47J::FObj());
  HoleDir.TestClock(Day2+3*3600+20*60);
  const bool bShut=HoleDir.HoleFrom<0;
  HoleDir.SetFlag(StarHoleFlag);
  const double Opened=HoleDir.HoleFrom;
  HoleDir.TestClock(Day2+4*3600);
  HoleDir.SetFlag(StarHoleFlag);
  FS47StoryDirector HoleBack(nullptr);
  HoleBack.BeginWith(ActsText,HoleDir.Save());
  Check(bShut&&FMath::IsNearlyEqual(Opened,3*3600+20*60+STAR_HOLE.After)&&HoleDir.HoleFrom==Opened&&FMath::IsNearlyEqual(HoleBack.HoleFrom,Opened)
   &&!StarHoleFrom(nullptr).IsSet()&&FMath::IsNearlyEqual(HoleFromAt(-60),86340+STAR_HOLE.After),
   TEXT("STORY-16: the star hole opens once with its flag, at the clock then and STAR_HOLE.After; its start keeps through a save; none without a Story night's director (never in Freeform)"));
 }
 { // STORY steg 6, the morning act: SC11 (the telex, the visit at SARO), SC12 (Ward), SC13 (the last road and the end)
  static const TCHAR* const MorningIds[]={TEXT("morning_saro"),TEXT("morning_telex"),TEXT("morning_ward"),TEXT("morning_road"),TEXT("morning_end")};
  TArray<const FS47EventScript*> Morning;
  for(const TCHAR* MorningId:MorningIds)Morning.Add(FindScript(MorningId));
  const FS47EventScript* Room6=FindScript(TEXT("night_room6_door")); // the door half sets room6_done last (night_room6 starts it at the motel)
  if(Morning.Contains(nullptr)||!Room6)Lines.Add(TEXT("NOTE story director: a morning_*.json of the morning act, or night_room6.json, is missing in Content/Director/Story/beats beside this build"));
  else
  {
   bool bLint=true;
   for(const FS47EventScript* Sc:Morning)bLint&=S47EventScript::LintStory(*Sc).IsEmpty()&&S47EventScript::Lint(*Sc).Num()>0;
   Check(bLint,TEXT("STORY-17: the morning act's scripts (morning_saro, morning_telex, morning_ward, morning_road, morning_end) pass the story's lint; Freeform's lint refuses each"));
   // The chain: night_room6 sets room6_done last; the morning act needs it and starts morning_saro; then telex, Ward or (taken) the end,
   // the road, the end. Each script starts only the next ones; the end is the last beat of morning_end and of no other story script.
   const auto StartsOf=[](const FS47EventScript& Sc){TArray<FString> Out;for(const FS47Beat& Bt:Sc.Beats)if(Bt.Do==ES47Beat::Start&&!Bt.Script.IsEmpty())Out.Add(Bt.Script);return Out;};
   const bool bRoom6Last=Room6->Beats.Num()>0&&Room6->Beats.Last().Do==ES47Beat::Flag&&Room6->Beats.Last().bSet&&Room6->Beats.Last().Flag==TEXT("room6_done")&&StartsOf(*Room6).IsEmpty();
   FString MorningText,MorningWhy;
   TArray<FAct> MorningActs;
   const bool bActOk=FFileHelper::LoadFileToString(MorningText,*ActsPath())&&ParseActs(MorningText,MorningActs,MorningWhy)&&MorningActs.Num()==3
    &&MorningActs[2].Id==TEXT("morning")&&MorningActs[2].Needs.Num()==1&&MorningActs[2].Needs[0]==TEXT("room6_done")&&MorningActs[2].Scripts.Num()==1&&MorningActs[2].Scripts[0]==TEXT("morning_saro");
   const TArray<FString> TelexNext=StartsOf(*Morning[1]);
   const bool bChain=StartsOf(*Morning[0])==TArray<FString>({TEXT("morning_telex")})&&TelexNext.Num()==2&&TelexNext.Contains(TEXT("morning_ward"))&&TelexNext.Contains(TEXT("morning_end"))
    &&StartsOf(*Morning[2])==TArray<FString>({TEXT("morning_road")})&&StartsOf(*Morning[3])==TArray<FString>({TEXT("morning_end")})&&StartsOf(*Morning[4]).IsEmpty();
   // The branch is the visit's: Ward when the player was not taken, the end at once when they were.
   const auto StartIf=[](const FS47EventScript& Sc,const TCHAR* To,const TCHAR* If){return Sc.Beats.ContainsByPredicate([To,If](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Script==To&&Bt.If.Num()==1&&Bt.If[0].If==If&&Bt.If[0].Arg==TakenFlag;});};
   const bool bBranch=StartIf(*Morning[1],TEXT("morning_ward"),TEXT("no_story_flag"))&&StartIf(*Morning[1],TEXT("morning_end"),TEXT("story_flag"));
   int32 Ends=0;
   for(const FS47EventScript& Any:StoryScripts())for(const FS47Beat& Bt:Any.Beats)Ends+=Bt.Do==ES47Beat::End?1:0;
   const bool bEndLast=Morning[4]->Beats.Num()>0&&Morning[4]->Beats.Last().Do==ES47Beat::End&&Ends==1;
   Check(bRoom6Last&&bActOk,TEXT("STORY-18: night_room6 sets room6_done as its last beat; the morning act (from 04:00) needs room6_done and starts morning_saro only"));
   Check(bChain&&bBranch&&bEndLast,TEXT("STORY-19: morning_saro, morning_telex, then Ward (not taken) or the end (taken), the road, the end; the end beat is morning_end's last and the only one"));
   // The real acts: the night holds at 05:00 until room 6 is done, then the morning act starts morning_saro (Tick(0): the act moves,
   // what it starts waits for the next real step).
   FS47StoryDirector Night(nullptr);
   Night.BeginWith(MorningText,S47J::Read(TEXT("{\"v\":1,\"act\":\"night\"}")));
   Night.TestClock(Day2+5*3600.);Night.Tick(0);
   const bool bHeldNight=Night.CurrentAct()&&Night.CurrentAct()->Id==TEXT("night")&&Night.Pending.IsEmpty();
   Night.SetFlag(TEXT("room6_done"));Night.Tick(0);
   const bool bMorning=Night.CurrentAct()&&Night.CurrentAct()->Id==TEXT("morning")&&Night.Pending.Num()==1&&Night.Pending[0].Id==TEXT("morning_saro");
   Check(bHeldNight&&bMorning,TEXT("STORY-18: with the real acts the night stays at 05:00 until room6_done, then the morning act starts morning_saro"));
   // SC11: the telex prints before the visit; the visit waits for the full tier (05:04 at least); its end and the taken flag are cleared first.
   const FS47EventScript& Telex=*Morning[1];
   const int32 PrintIdx=Telex.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Telex&&!Bt.Key.IsEmpty()&&Bt.Text.Contains(TEXT("05:29:47"))&&Bt.Text.Contains(TEXT("S-11"));});
   const int32 VisitIdx=Telex.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Start&&Bt.Happening==AttentionVisitId;});
   const int32 TierIdx=Telex.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Clock&&Bt.AtLeast>=29.;});
   const int32 TakenClear=Telex.Beats.IndexOfByPredicate([](const FS47Beat& Bt){return Bt.Do==ES47Beat::Flag&&!Bt.bSet&&Bt.Flag==TakenFlag;});
   Check(PrintIdx>=0&&VisitIdx>PrintIdx&&TierIdx>=0&&TierIdx<VisitIdx&&TakenClear>=0&&TakenClear<VisitIdx,
    TEXT("STORY-20: morning_telex prints the remote sheets (H-1, S-11, 05:29:47) on Freeform's telex, then starts the visit at SARO after 05:04 with the taken flag cleared"));
   // The waits on a Freeform end follow their start and a clearing of the flag, as in the night act; nothing in the sky, no Dale line.
   bool bEnds=true,bQuiet=true;int32 Waits=0;
   static const TCHAR* const MorningSky[]={TEXT("flyover"),TEXT("close_pass"),TEXT("sky_formation"),TEXT("sky_high"),TEXT("landing"),TEXT("response_lights"),TEXT("dusk_orb"),TEXT("highway_ufo"),TEXT("farm_lights"),TEXT("town_lights")};
   for(const FS47EventScript* Sc:Morning)
   {
    for(int32 K=0;K<Sc->Beats.Num();++K)
    {
     const FS47Beat& Bt=Sc->Beats[K];
     bQuiet&=!(Bt.Do==ES47Beat::Flag&&Bt.Flag==StarHoleFlag)&&!(Bt.Do==ES47Beat::Line&&Bt.Who==TEXT("dale"))&&Bt.Do!=ES47Beat::Scene;
     for(const TCHAR* SkyId:MorningSky)bQuiet&=!(Bt.Do==ES47Beat::Start&&(Bt.Event==SkyId||Bt.Happening==SkyId));
     for(const FS47ScriptCond& C:Bt.Wait)
     {
      if(C.If!=TEXT("story_flag")||!C.Arg.StartsWith(TEXT("ended_")))continue;
      ++Waits;
      const FString EndedId=C.Arg.Mid(6);
      bool bStarted=false,bCleared=false;
      for(int32 J=0;J<K;++J)
      {
       const FS47Beat& Prev=Sc->Beats[J];
       bStarted|=Prev.Do==ES47Beat::Start&&(Prev.Event==EndedId||Prev.Happening==EndedId);
       bCleared|=Prev.Do==ES47Beat::Flag&&!Prev.bSet&&Prev.Flag==C.Arg;
      }
      bEnds&=bStarted&&bCleared;
     }
    }
   }
   Check(bEnds&&Waits==1&&bQuiet,TEXT("STORY-21: the wait on the visit's end follows its start and a clearing of ended_attention_visit; the morning starts nothing in the sky, leaves the star hole as it is and gives Dale no line"));
  }
  // The end beat and the telex beat: story-only, read, linted; the end is due once, holds the night, keeps through a save.
  {
   FS47EventScript EndRead;FString EndWhy;
   const bool bReadEnd=ReadScript(TEXT("{\"version\":1,\"id\":\"story_end_probe\",\"name\":\"probe\",\"beats\":[{\"at\":1,\"do\":\"telex\",\"key\":\"telex-probe\",\"title\":\"A sheet\",\"stamp\":\"TELEX\",\"text\":\"A page.\"},{\"at\":2,\"do\":\"end\"}]}"),EndRead,EndWhy)
    &&EndRead.Beats.Num()==2&&EndRead.Beats[0].Do==ES47Beat::Telex&&EndRead.Beats[0].Name==TEXT("A sheet")&&EndRead.Beats[0].Kind==TEXT("TELEX")&&EndRead.Beats[1].Do==ES47Beat::End;
   FS47EventScript NoTitle;FString NoTitleWhy;
   const bool bRefuses=!ReadScript(TEXT("{\"version\":1,\"id\":\"story_end_probe\",\"name\":\"probe\",\"beats\":[{\"at\":1,\"do\":\"telex\",\"key\":\"telex-probe\",\"text\":\"A page.\"}]}"),NoTitle,NoTitleWhy)
    &&!ReadScript(TEXT("{\"version\":1,\"id\":\"story_end_probe\",\"name\":\"probe\",\"beats\":[{\"at\":1,\"do\":\"telex\",\"title\":\"A sheet\",\"text\":\"A page.\"}]}"),NoTitle,NoTitleWhy);
   Check(bReadEnd&&S47EventScript::Lint(EndRead).Num()>0&&S47EventScript::StoryOnlyBeat(ES47Beat::Telex)&&S47EventScript::StoryOnlyBeat(ES47Beat::End)&&bRefuses,
    TEXT("STORY-22: telex and end are story-only beats: the story's lint takes them (telex with key, title and text), Freeform's refuses them"));
   FS47StoryDirector EndDir(nullptr);
   EndDir.BeginWith(ActsText,S47J::FObj());
   EndDir.StartPending();
   const bool bIdleFirst=!EndDir.Busy()&&!EndDir.IsEndDue();
   FS47Beat EndBeat;EndBeat.Do=ES47Beat::End;
   EndDir.Apply(EndBeat,FString());
   const bool bDue=EndDir.IsEndDue()&&EndDir.Busy()&&!EndDir.HasEnded()&&EndDir.Apply(EndBeat,FString()).Contains(TEXT("already"));
   EndDir.Tick(.1); // no game: the end waits, still due
   const S47J::FObj EndSaved=S47J::Read(S47J::Write(EndDir.Save()));
   FS47StoryDirector EndBack(nullptr);
   EndBack.BeginWith(ActsText,EndSaved);
   FS47StoryDirector Done(nullptr);
   Done.BeginWith(ActsText,S47J::Read(TEXT("{\"v\":1,\"act\":\"morning\",\"ending\":true,\"flags\":[\"story_end\"]}")));
   Check(bIdleFirst&&bDue&&EndDir.IsEndDue()&&S47J::Bool(EndSaved,TEXT("ending"),false)&&EndBack.IsEndDue()&&EndBack.Busy()&&!Done.IsEndDue(),
    TEXT("STORY-23: the end beat is due once and holds the night until it is played; it keeps through a save, and not once the end has been played"));
   Check(EndMay(false,false)&&!EndMay(true,false)&&!EndMay(false,true),TEXT("STORY-23: the end waits for a closed panel and for nobody being taken"));
   // The visit for the story: refused without a Story night (never in Freeform), and its end told at once when it cannot begin.
   FS47Attention NoNight(nullptr);
   FString VisitWhy;
   const bool bNoVisit=!NoNight.StartForStory(VisitWhy)&&!VisitWhy.IsEmpty()&&!NoNight.Visiting();
   FS47StoryDirector VisitDir(nullptr);
   VisitDir.BeginWith(ActsText,S47J::FObj());
   FS47Beat VisitBeat;VisitBeat.Do=ES47Beat::Start;VisitBeat.Happening=AttentionVisitId;
   const FString VisitSaid=VisitDir.Apply(VisitBeat,FString());
   Check(bNoVisit&&VisitDir.HasFlag(TEXT("ended_attention_visit"))&&VisitSaid.Contains(TEXT("did not begin"))&&EndedFlagOf(AttentionVisitId,FString())==TEXT("ended_attention_visit"),
    TEXT("STORY-24: the visit at SARO starts for the story only on a Story night; when it cannot begin its end is told at once, so the morning goes on"));
  }
 }
 return OK;
}
