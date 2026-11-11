#include "S47ScriptPlayer.h"
#include "SS47ScriptEditor.h"
#include "S47Audio.h"
#include "S47DevEvents.h"
#include "S47Docs.h"
#include "S47EventPlaces.h"
#include "S47EventScript.h"
#include "S47FarmWorld.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47Main.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47PaleBody.h"
#include "S47Radio.h"
#include "S47Shot.h"
#include "S47Sky.h"
#include "S47SkyVisitors.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47TripoCreature.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47World.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Script,Log,All);

// DIRECTOR-TOOL-20261109: the scripted events' player (S47ScriptPlayer.h).

struct FS47ScriptPlayer::FBodies
{
 // pale, tall, crab (S47Script::Bodies), each built once on the root and hidden when not in use.
 FS47PaleBody Body[3];
 bool bInUse[3]={false,false,false};
};

namespace S47ScriptPlayerLocal
{
 TMap<const US47Main*,FS47ScriptPlayer*>& Registry(){static TMap<const US47Main*,FS47ScriptPlayer*> R;return R;}
 constexpr double CheckEvery=2.,LookDegrees=14.,SeenSeconds=.3,SafetySeconds=180.;
 FString Clean(const FString& Path){FString P=Path;FPaths::NormalizeDirectoryName(P);FPaths::CollapseRelativeDirectories(P);return P;}
 ES47SkyForm FormOf(const FString& Name)
 {
  if(Name==TEXT("line"))return ES47SkyForm::Line;
  if(Name==TEXT("triangle"))return ES47SkyForm::Triangle;
  if(Name==TEXT("circle"))return ES47SkyForm::Circle;
  if(Name==TEXT("scatter"))return ES47SkyForm::Scatter;
  return ES47SkyForm::V;
 }
}

FS47ScriptPlayer::FS47ScriptPlayer(US47Main* Main):M(Main),Bodies(MakeUnique<FBodies>())
{
 S47ScriptPlayerLocal::Registry().Add(Main,this);
 Beats=MakeUnique<FS47EventBeats>(Main); // D46-20261110
}

FS47ScriptPlayer::~FS47ScriptPlayer()
{
 auto& R=S47ScriptPlayerLocal::Registry();
 if(R.FindRef(M)==this)R.Remove(M);
}

FS47ScriptPlayer* FS47ScriptPlayer::Of(const US47Main* InM){return InM?S47ScriptPlayerLocal::Registry().FindRef(InM):nullptr;}
bool FS47ScriptPlayer::Busy(const US47Main* InM){const FS47ScriptPlayer* P=Of(InM);return P&&P->Running();}
TSet<FString>& FS47ScriptPlayer::Holders(){static TSet<FString> H;return H;}
void FS47ScriptPlayer::Hold(const FString& Who,bool bOn){if(bOn)Holders().Add(Who);else Holders().Remove(Who);}

bool FS47ScriptPlayer::DevAllowed(const US47Main* InM)
{
#if UE_BUILD_SHIPPING
 return false;
#else
 return (InM&&InM->Flow&&InM->Flow->bDevRun)||FParse::Param(FCommandLine::Get(),TEXT("S47Director"));
#endif
}

TArray<FString> FS47ScriptPlayer::ReadDirs()
{
 TArray<FString> Dirs={FPaths::ProjectContentDir()/TEXT("Director/scripted"),FPaths::ProjectSavedDir()/TEXT("Director/scripted")};
 FString Own;
 if(FParse::Value(FCommandLine::Get(),TEXT("S47ScriptDir="),Own)&&!Own.IsEmpty())Dirs.Add(Own);
 for(FString& D:Dirs)D=S47ScriptPlayerLocal::Clean(D);
 TArray<FString> Out;for(const FString& D:Dirs)Out.AddUnique(D);
 return Out;
}

FString FS47ScriptPlayer::WriteDir()
{
 FString Own;
 if(FParse::Value(FCommandLine::Get(),TEXT("S47ScriptDir="),Own)&&!Own.IsEmpty())return S47ScriptPlayerLocal::Clean(Own);
 // A packaged game's Content is in its pak: what is written there would never be read over it.
 return S47ScriptPlayerLocal::Clean(GIsEditor?FPaths::ProjectContentDir()/TEXT("Director/scripted"):FPaths::ProjectSavedDir()/TEXT("Director/scripted"));
}

FS47FreeformSession* FS47ScriptPlayer::Session() const{return M&&M->IsFreeform()&&M->Freeform&&M->Freeform->Active()?M->Freeform.Get():nullptr;}

void FS47ScriptPlayer::Begin(const S47J::FObj& Saved)
{
 StopRun();
 if(Beats)Beats->Stop();
 S47EventScript::Reload(); // D46-20261110: the event scripts, read again with each session
 Memory.Load(S47J::Obj(Saved,TEXT("played")));
 Reload();
 CheckIn=4;
 AddDevLines();
}

void FS47ScriptPlayer::Stop()
{
 StopRun();
 if(Beats)Beats->Stop();
 if(Bodies)for(int32 K=0;K<3;++K)if(Bodies->Body[K].bBuilt)Bodies->Body[K].Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);
}

S47J::FObj FS47ScriptPlayer::Save() const
{
 const S47J::FObj O=S47J::New();
 if(Memory.Count.Num())S47J::Set(O,TEXT("played"),Memory.Save());
 return O;
}

void FS47ScriptPlayer::Reload()
{
 TMap<FString,FS47Script> ById;
 Bad.Reset();
 for(const FString& Dir:ReadDirs())
 {
  TArray<FString> Files;
  IFileManager::Get().FindFiles(Files,*(Dir/TEXT("*.json")),true,false);
  Files.Sort();
  for(const FString& Name:Files)
  {
   const FString Path=Dir/Name;
   FString Text,Why;FS47Script S;
   if(!FFileHelper::LoadFileToString(Text,*Path)){Bad.Add(Path,TEXT("could not be read"));continue;}
   if(!S47Script::Read(Text,S,Why)){Bad.Add(Path,Why);UE_LOG(LogS47Script,Warning,TEXT("S47: script %s refused: %s"),*Path,*Why);continue;}
   S.File=Path;
   const TArray<FString> Errors=S47Script::Lint(S);
   if(Errors.Num()){Bad.Add(S.Id,Errors[0]);UE_LOG(LogS47Script,Warning,TEXT("S47: script %s will not play: %s"),*S.Id,*Errors[0]);}
   else Bad.Remove(S.Id);
   ById.Add(S.Id,S);
  }
 }
 Scripts.Reset();
 for(auto& P:ById)Scripts.Add(P.Value);
 Scripts.Sort([](const FS47Script& A,const FS47Script& B){return A.Id<B.Id;});
 int32 On=0;for(const FS47Script& S:Scripts)On+=S.bEnabled&&!Bad.Contains(S.Id);
 UE_LOG(LogS47Script,Display,TEXT("S47: scripts read: %d, %d switched on and clean, %d refused"),Scripts.Num(),On,Bad.Num());
}

const FS47Script* FS47ScriptPlayer::Find(const FString& Id) const
{
 return Scripts.FindByPredicate([&Id](const FS47Script& S){return S.Id==Id;});
}

bool FS47ScriptPlayer::Store(const FS47Script& S,FString& Why)
{
 const TArray<FString> Errors=S47Script::Lint(S);
 if(Errors.Num()){Why=Errors[0];return false;}
 const FString Dir=WriteDir();
 IFileManager::Get().MakeDirectory(*Dir,true);
 const FString Path=Dir/(S.Id+TEXT(".json"));
 if(!FFileHelper::SaveStringToFile(S47Script::Write(S)+TEXT("\n"),*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)){Why=TEXT("could not write ")+Path;return false;}
 UE_LOG(LogS47Script,Display,TEXT("S47: script %s saved to %s"),*S.Id,*Path);
 Reload();
 Why=Path;
 return true;
}

void FS47ScriptPlayer::AddDevLines()
{
 // D46-20261110 (D46-11): the event scripts' lines are there in any Freeform night, not only a developer's run.
 if(Beats)Beats->AddDevLines();
 if(bLinesAdded||!M||!M->DevEvents||!DevAllowed(M))return;
 S47Shot::AddDevLine(M); // SHOT-BEAT-01: the F10 group "Director shot" (it looks for itself whether it is there)
 static const TCHAR* const Group=TEXT("Director scripts");
 bLinesAdded=true;
 // The menu outlives a session: its lines are added once.
 if(M->DevEvents->Events().ContainsByPredicate([](const FS47DevEvents::FEvent& E){return E.Group==Group;}))return;
 US47Main* Main=M;
 M->DevEvents->AddFreeform(Group,TEXT("Script editor"),TEXT("make, try and save scripted events; points where you stand or look"),[Main]{S47ScriptEditor::Open(Main);});
 M->DevEvents->AddFreeform(Group,TEXT("Stop the playing script"),TEXT("everything it started goes"),[Main]
 {
  FS47ScriptPlayer* P=Of(Main);
  if(P)P->StopRun(TEXT("F10"));
  if(Main->UI)Main->UI->Toast(P?TEXT("Developer event: the script is stopped."):TEXT("Developer event: no script player here."),3);
 });
 M->DevEvents->AddFreeform(Group,TEXT("Read the scripts again"),TEXT("from Content/Director/scripted and your own folder"),[Main]
 {
  FS47ScriptPlayer* P=Of(Main);
  if(P)P->Reload();
  if(Main->UI)Main->UI->Toast(P?FString::Printf(TEXT("Developer event: %d scripts read, %d refused."),P->Library().Num(),P->Refused().Num()):FString(TEXT("Developer event: no script player here.")),4);
 });
}

bool FS47ScriptPlayer::BigEvent(const US47Main* InM,FString& Why)
{
 Why.Reset();
 if(!InM)return false;
 if(Holders().Num()){Why=TEXT("held by ")+FString::Join(Holders().Array(),TEXT(", "));return true;}
 if(InM->HighwayUfo&&InM->HighwayUfo->Busy()){Why=TEXT("the light over the highway");return true;}
 const FS47FreeformSession* F=InM->Freeform.Get();
 if(!F)return false;
 if(F->Director&&F->Director->Busy()){Why=TEXT("the Director's event ")+F->Director->ActiveEvent();return true;}
 if(F->Taken&&F->Taken->Busy()){Why=TEXT("being taken");return true;}
 if(F->Happenings&&!F->Happenings->Ground().IsEmpty()){Why=TEXT("the night's ")+F->Happenings->Ground();return true;}
 return false;
}

FS47ScriptNow FS47ScriptPlayer::Now() const
{
 FS47ScriptNow N;
 FS47FreeformSession* F=Session();
 if(!F)return N;
 N.Clock=F->Clock();N.Day=F->Day();N.Night=FS47FreeformSession::NightOf(N.Clock);
 FS47ScriptPoint Here;
 if(HereRoad(Here)){N.bHasRoad=true;N.Road=Here.Road();}
 if(M->Player&&M->World)N.bIndoors=M->World->Indoors(M->Player->Pos);
 N.bInTruck=M->World&&M->World->bDriving;
 // The weather and the moon as the world shows them now (SkyNow): the calendar day's, not the shift day's.
 SkyNow(F->Seed(),N.Clock,N.Day,F->DateOfDay(CalendarDayOf(N.Clock)),N);
 N.HasFinding=[F](const FString& Id)
 {
  if(!Id.EndsWith(TEXT("*")))return F->HasFinding(Id);
  const FString Head=Id.LeftChop(1);
  for(const FString& K:F->FindingIdsFrom(0))if(K.StartsWith(Head))return true;
  return false;
 };
 N.CaseState=[F](int32 Line)->int32
 {
  const FS47FreeformCases::FCase* C=F->CaseFile().Get(Line);
  return !C?0:C->bConcluded?2:C->bOpen?1:0;
 };
 N.PlaceAt=[F](const FString& Id,FVector2D& At,double& R)
 {
  const FS47EventPlace* P=F->EventPlaces?F->EventPlaces->Find(Id):nullptr;
  if(!P)return false;
  At=FVector2D(P->X,P->Z);R=P->Radius;return true;
 };
 N.bBigEvent=BigEvent(M,N.BigWhy);
 return N;
}

void FS47ScriptPlayer::SkyNow(int32 Seed,double Clock,int32 ShiftDay,const FDateTime& CalendarDate,FS47ScriptNow& N)
{
 // CRIT6-20261109: what FS47WeatherLive puts on the world (S47Weather::Now on the calendar day) and the cover the sky draws with it
 // (FS47Sky::CloudCoverAt and the day's CloudBias, as S47Weather::Day reckons its own); between midnight and eight the shift day is
 // still yesterday, whose morning cover is not tonight's.
 const int32 CalDay=CalendarDayOf(Clock);
 const double Sec=FMath::Fmod(FMath::Max(0.,Clock),86400.),H=Sec/3600.;
 const S47Weather::FNow W=S47Weather::Now(Seed,CalDay,Sec);
 const double Cover=FMath::Clamp(FS47Sky::CloudCoverAt(Sec)+W.CloudBias,0.,.9);
 N.Weather.Reset();
 N.Weather.Add(Cover>=.5?TEXT("cloudy"):TEXT("clear"));
 N.Weather.Add(W.Mph>=18?TEXT("windy"):TEXT("calm"));
 if(W.Dust>.1)N.Weather.Add(TEXT("dust"));
 if(W.Rain>0)N.Weather.Add(TEXT("shower"));
 // The storm is the evening's wind held into the night: the shift day's, which in the small hours is the evening before.
 if(S47Weather::Day(Seed,FMath::Max(1,ShiftDay)).NightStorm>.5f&&(H>=19||H<6))N.Weather.Add(TEXT("storm"));
 const S47Weather::FMoon Mo=S47Weather::Moon(CalendarDate,Sec);
 N.MoonLit=Mo.Lit;N.bMoonUp=Mo.El>0;
}

FString FS47ScriptPlayer::WhyNot(const FS47Script& S) const
{
 FString Why;
 if(!Session())return TEXT("not in Freeform");
 if(bRun)return TEXT("another script is playing");
 return S47Script::Ready(S,Now(),Memory,Why)?FString():Why;
}

bool FS47ScriptPlayer::HereRoad(FS47ScriptPoint& Out) const
{
 if(!M||!M->Player||!M->Web)return false;
 double X=0,Z=0;
 if(!S47OpenLand::RoadOf(M->Web->Area(),M->Player->Pos.X,M->Player->Pos.Z,X,Z))return false;
 Out=FS47ScriptPoint();Out.X=X;Out.Z=Z;Out.bSet=true;
 return true;
}

FVector FS47ScriptPlayer::EyeNow() const
{
 return M&&M->Player?FVector(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z):FVector::ZeroVector;
}

bool FS47ScriptPlayer::LookRoad(FS47ScriptPoint& Out) const
{
 if(!M||!M->Player||!M->Web||!M->Web->GetWorld())return false;
 const double Yaw=M->Player->Yaw,Pitch=M->Player->Pitch;
 const FVector Dir(-FMath::Sin(Yaw)*FMath::Cos(Pitch),FMath::Sin(Pitch),-FMath::Cos(Yaw)*FMath::Cos(Pitch));
 const FVector Eye=EyeNow();
 FVector Hit=Eye+Dir*400.;
 FHitResult R;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ScriptLook),true);
 if(M->Web->GetWorld()->LineTraceSingleByChannel(R,S47Three::ToUE(Eye),S47Three::ToUE(Hit),ECC_Visibility,Q))Hit=S47Three::ToThree(R.ImpactPoint);
 else if(Dir.Y<-.02)Hit=Eye+Dir*FMath::Min(400.,M->Player->Eye/-Dir.Y); // no hit: where the view meets the ground's height
 double X=0,Z=0;
 if(!S47OpenLand::RoadOf(M->Web->Area(),Hit.X,Hit.Z,X,Z))return false;
 Out=FS47ScriptPoint();Out.X=X;Out.Z=Z;Out.bSet=true;
 // What is looked at above the ground (a roof, the sky's line) is kept as its height.
 Out.Up=FMath::Max(0.,FMath::RoundToDouble((Hit.Y-S47FarmWorld::FloorRoad(X,Z))*10)/10);
 return true;
}

FVector FS47ScriptPlayer::WorldOf(const FS47ScriptPoint& P) const
{
 if(!M||!M->Web)return FVector::ZeroVector;
 FVector At=S47OpenLand::FromRoad(M->Web->Area(),P.X,P.Z);
 At.Y=S47FarmWorld::FloorRoad(P.X,P.Z)+P.Up;
 return At;
}

bool FS47ScriptPlayer::LookingAt(const FVector& At,double Degrees) const
{
 if(!M||!M->Player)return false;
 const FVector D=(At-EyeNow()).GetSafeNormal();
 const double Yaw=M->Player->Yaw,Pitch=M->Player->Pitch;
 const FVector Fwd(-FMath::Sin(Yaw)*FMath::Cos(Pitch),FMath::Sin(Pitch),-FMath::Cos(Yaw)*FMath::Cos(Pitch));
 return FVector::DotProduct(D,Fwd)>=FMath::Cos(FMath::DegreesToRadians(Degrees));
}

bool FS47ScriptPlayer::Preview(const FS47Script& S,FString& Why)
{
 if(!Session()){Why=TEXT("not in Freeform");return false;}
 if(bRun){Why=TEXT("another script is playing: ")+Run.S.Id;return false;}
 if(!S47Script::Ready(S,Now(),Memory,Why,true))return false;
 const FString Said=Why;
 if(!Start(S,true,Why))return false;
 Why=Said;
 return true;
}

bool FS47ScriptPlayer::Start(const FS47Script& S,bool bPreview,FString& Why)
{
 FS47FreeformSession* F=Session();
 if(!F||!M->Web){Why=TEXT("no world");return false;}
 ClearEffects();
 Run=FRun();Run.S=S;Run.bPreview=bPreview;bRun=true;
 // The timeline in order (the editor's draft may not be): the same start keeps the order it was given.
 Run.S.Actions.StableSort([](const FS47ScriptAction& X,const FS47ScriptAction& Y){return X.At<Y.At;});
 if(!bPreview)Memory.Played(S.Id,FS47FreeformSession::NightOf(F->Clock()),F->Clock());
 UE_LOG(LogS47Script,Display,TEXT("S47: script %s starts%s (day %d, %s, %d actions)"),*S.Id,bPreview?TEXT(" as a preview"):TEXT(""),F->Day(),*S47Script::HourText(FMath::Fmod(F->Clock(),86400.)/3600.),S.Actions.Num());
 return true;
}

void FS47ScriptPlayer::StopRun(const FString& Why)
{
 if(bRun)UE_LOG(LogS47Script,Display,TEXT("S47: script %s stopped%s%s"),*Run.S.Id,Why.IsEmpty()?TEXT(""):TEXT(": "),*Why);
 bRun=false;
 ClearEffects();
}

void FS47ScriptPlayer::Finish(const FString& Why)
{
 UE_LOG(LogS47Script,Display,TEXT("S47: script %s over (%s) after %.0f s"),*Run.S.Id,*Why,Run.Age);
 bRun=false;
 ClearEffects();
}

void FS47ScriptPlayer::RestoreDishes()
{
 if(!DishHold.bLive||!M||!IsValid(M->Web))return;
 // CRIT6-20261109: the big night's bearing is the array's now (its toast and its finding say so): a script ending does not undo it.
 const FS47FreeformSession* F=M->Freeform.Get();
 const FS47Happenings* H=F?F->Happenings.Get():nullptr;
 if(H&&(H->BigLive()||(H->BigHas(TEXT("turned"))&&!DishHold.bBigTurned)))
 {
  UE_LOG(LogS47Script,Display,TEXT("S47: script %s: the array is left where the big night turned it"),*Run.S.Id);
  return;
 }
 for(int32 I=0;I<DishHold.Targets.Num();++I)M->Web->PointDish(I,DishHold.Targets[I].X,DishHold.Targets[I].Y,9,true);
}

bool FS47ScriptPlayer::EffectsLive() const
{
 return (Shot&&Shot->Live())||Look.bLive||DishHold.bLive||SkyLeft>0||OmenLeft>0||Figures.ContainsByPredicate([](const FFigure& F){return F.bLive;});
}

void FS47ScriptPlayer::ClearEffects()
{
 for(FFigure& F:Figures)if(F.bLive&&Bodies)
 {
  FS47PaleBody& B=Bodies->Body[F.Body];
  if(B.bBuilt)B.Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);
 }
 Figures.Reset();
 if(Bodies)for(bool& U:Bodies->bInUse)U=false;
 Look=FLook();
 if(Shot)Shot->Stop(); // SHOT-BEAT-01: the camera as it was before the shot, also mid-shot
 RestoreDishes();
 DishHold=FDishHold();
 if(bSkyStarted&&M&&M->SkyVisitors)M->SkyVisitors->DevFormationStop();
 bSkyStarted=false;SkyLeft=0;
 if(bOmenPosted&&M&&M->Omen){FS47OmenSource Gone;Gone.Id=TEXT("script:")+Run.S.Id;M->Omen->Post(Gone,0);}
 bOmenPosted=false;OmenLeft=0;
}

void FS47ScriptPlayer::Step(double Dt)
{
 if(!M)return;
 AddDevLines();
 if(Shot)Shot->Tick(); // SHOT-BEAT-01: a shot that is over gives the walker its camera back (outside the walker's frame)
 FS47FreeformSession* F=Session();
 if(!F){if(bRun)StopRun(TEXT("left Freeform"));if(Beats&&Beats->Live())Beats->Stop();return;}
 if(F->Held()||Dt<=0)return; // a menu, a paper, a held scene: the timeline waits where it is
 if(Beats)Beats->Step(Dt); // D46-20261110: the beats of the night's events (S47EventScript.h)
 if(bRun)
 {
  Advance(Dt);
  for(FFigure& Fig:Figures)if(Fig.bLive)StepFigure(Fig,Dt);
  StepLook(Dt);
  SkyLeft=FMath::Max(0.,SkyLeft-Dt);OmenLeft=FMath::Max(0.,OmenLeft-Dt);
  if(DishHold.bLive&&(DishHold.Left-=Dt)<=0)
  {
   RestoreDishes();
   DishHold=FDishHold();
  }
  if(bRun&&Run.Next>=Run.S.Actions.Num()&&Run.Hold<=0&&!EffectsLive())Finish(TEXT("done"));
  else if(bRun&&Run.Age>Run.S.Length()+S47ScriptPlayerLocal::SafetySeconds)Finish(TEXT("ran too long"));
  return;
 }
 // CRIT6-20261109: nothing starts in the first shift's tutorial, nor where the night's own rules do not run (a developer's fixtures,
 // the scenario tests); PREVIEW still plays.
 if(F->InTutorial()||!F->RulesLive())return;
 if((CheckIn-=Dt)>0)return;
 CheckIn=S47ScriptPlayerLocal::CheckEvery;
 bool bAnyOn=false;
 for(const FS47Script& S:Scripts)bAnyOn|=S.bEnabled;
 if(!bAnyOn)return;
 const FS47ScriptNow N=Now();
 for(const FS47Script& S:Scripts)
 {
  FString Why;
  if(!S.bEnabled||Bad.Contains(S.Id)||!S47Script::Ready(S,N,Memory,Why))continue;
  if(Start(S,false,Why))break;
 }
}

void FS47ScriptPlayer::Advance(double Dt)
{
 Run.Age+=Dt;
 if(Run.Hold>0)
 {
  Run.Hold-=Dt;
  if(Run.bHoldLook&&Run.HoldAt.bSet&&LookingAt(WorldOf(Run.HoldAt),S47ScriptPlayerLocal::LookDegrees))Run.Hold=0;
  if(Run.Hold>0)return;
  Run.bHoldLook=false;
 }
 Run.T+=Dt;
 while(bRun&&Run.Next<Run.S.Actions.Num()&&Run.S.Actions[Run.Next].At<=Run.T)
 {
  const FS47ScriptAction A=Run.S.Actions[Run.Next++];
  Fire(A);
  if(A.Do==ES47ScriptAct::Wait)break;
 }
}

void FS47ScriptPlayer::Fire(const FS47ScriptAction& A)
{
 FS47FreeformSession* F=Session();
 if(!F)return;
 const FString& Id=Run.S.Id;
 const bool bKeeps=!Run.bPreview||(M->Flow&&M->Flow->bDevRun); // a preview in a real game leaves nothing in its save
 const auto Log=[&Id,&A](const FString& What){UE_LOG(LogS47Script,Display,TEXT("S47: script %s at %.1f s: %s %s"),*Id,A.At,S47Script::ActName(A.Do),*What);};
 switch(A.Do)
 {
 case ES47ScriptAct::Figure:
 {
  const int32 K=S47Script::Bodies().IndexOfByKey(A.Kind);
  if(K<0||A.Path.IsEmpty()||!Bodies){Log(TEXT("refused: no body or no path"));break;}
  if(Bodies->bInUse[K]){Log(TEXT("refused: that body is already out in this script"));break;}
  FS47PaleBody& B=Bodies->Body[K];
  if(!B.bBuilt)
  {
   static const TCHAR* const Prefix[3]={TEXT("ScriptPale"),TEXT("ScriptTall"),TEXT("ScriptCrab")};
   B.CreatureAsset=K==1?S47KindBody::Tall:K==2?S47KindBody::Crab:S47KindBody::Pale;
   if(K==1)B.Style=ES47PaleStyle::Tall;
   B.Build(M->Web.Get(),TEXT("#root"),Prefix[K]);
  }
  if(!B.bBuilt||(K>0&&!B.Creature.Ready())){Log(TEXT("refused: the body could not be built here (the private skinned model?)"));break;}
  FFigure Fig;Fig.Body=K;Fig.Path=A.Path;Fig.Speed=FMath::Clamp(A.Speed,.2,6.);Fig.Stand=FMath::Max(0.,A.Secs);Fig.bVanish=A.bFlag;Fig.bLive=true;
  Fig.RoadX=A.Path[0].X;Fig.RoadZ=A.Path[0].Z;Fig.Y=WorldOf(A.Path[0]).Y;Fig.Seg=1;
  if(A.Path.Num()>1){const FVector P0=WorldOf(A.Path[0]),P1=WorldOf(A.Path[1]);Fig.Yaw=FMath::Atan2(P1.Z-P0.Z,P1.X-P0.X);}
  Bodies->bInUse[K]=true;
  Figures.Add(Fig);
  Log(FString::Printf(TEXT("%s, %d points"),*A.Kind,A.Path.Num()));
  break;
 }
 case ES47ScriptAct::Sky:
 {
  if(!M->SkyVisitors){Log(TEXT("refused: no sky"));break;}
  const int32 Pace=FMath::Max(0,S47Script::Paces().IndexOfByKey(A.Who));
  const bool bShown=A.Kind==TEXT("high_track")?M->SkyVisitors->DevHighTrack():M->SkyVisitors->DevFormation(S47ScriptPlayerLocal::FormOf(A.Kind),Pace);
  bSkyStarted|=bShown&&A.Kind!=TEXT("high_track");
  if(bShown&&A.Kind!=TEXT("high_track"))SkyLeft=FMath::Max(SkyLeft,Pace==1?24.:95.);
  Log(bShown?A.Kind:FString(TEXT("refused: not outdoors")));
  break;
 }
 case ES47ScriptAct::Sound:
 {
  if(!M->Audio){Log(TEXT("refused: no sound"));break;}
  FS47Sound O;O.Gain=float(FMath::Clamp(A.Gain,0.,2.));
  if(A.Who==TEXT("placed")&&A.Point.bSet)O.At=WorldOf(A.Point);
  else if(A.Who==TEXT("radio"))O.Dest=TEXT("radio"); // the truck's radio speaker, when it is on
  else if(A.Who==TEXT("cb")){if(!FS47FieldRadio::CbAtHand(M)){Log(TEXT("not heard: no CB at hand"));break;}O.Dest=TEXT("radioBus");O.Lp=2600;}
  else if(A.Who==TEXT("walkie")){if(!M->FieldRadio||!M->FieldRadio->Owned()){Log(TEXT("not heard: no walkie"));break;}O.Lp=2200;O.Gain*=.7f;}
  M->Audio->Sfx(A.Text,O);
  Log(A.Text+TEXT(" ")+A.Who);
  break;
 }
 case ES47ScriptAct::Dish:
 {
  if(!M->Web||F->PowerFault()){Log(TEXT("refused: no power on the array"));break;}
  if(!DishHold.bLive){DishHold.Targets.Reset();for(int32 I=0;I<M->Web->DishCount();++I)DishHold.Targets.Add(FVector2D(M->Web->DishTargetAz(I),M->Web->DishTargetEl(I)));DishHold.bBigTurned=F->Happenings&&F->Happenings->BigHas(TEXT("turned"));}
  if(A.Index>=0&&A.Index<M->Web->DishCount())M->Web->PointDish(A.Index,A.Az,FMath::Clamp(A.El,0.,90.),FMath::Max(.5,A.Slew),true);
  else M->Web->PointDishes(A.Az,FMath::Clamp(A.El,0.,90.),FMath::Max(.5,A.Slew));
  if(A.Secs>0){DishHold.bLive=true;DishHold.Left=A.Secs;}else DishHold=FDishHold();
  Log(FString::Printf(TEXT("%s az %.0f el %.0f"),A.Index>=0?*FString::Printf(TEXT("antenna %d"),A.Index):TEXT("the array"),A.Az,A.El));
  break;
 }
 case ES47ScriptAct::Fault:
 {
  if(A.Kind==TEXT("power"))
  {
   if(bKeeps){F->SetPowerFault(true);Log(TEXT("the feeder"));}
   else Log(TEXT("preview in a real game: the feeder is left alone"));
  }
  else if(M->Omen)
  {
   if(A.Kind==TEXT("contact"))M->Omen->DevOrdinary(ES47Ordinary::Contact,FMath::Max(1.,A.Secs));
   else
   {
    FS47OmenSource Src;Src.Id=TEXT("script:")+Id;
    Src.At=A.Point.bSet?WorldOf(A.Point):(M->Player?FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z):FVector::ZeroVector);
    Src.Severity=FMath::Clamp(A.Gain,0.,1.);Src.Reach=FS47Omen::ReachFor(Src.Severity);
    M->Omen->Post(Src,FMath::Max(1.,A.Secs));bOmenPosted=true;OmenLeft=FMath::Max(OmenLeft,FMath::Max(1.,A.Secs));
   }
   Log(A.Kind);
  }
  break;
 }
 case ES47ScriptAct::Line:
 {
  bool bSaid=false;
  if(A.Who==TEXT("dale"))bSaid=M->FieldRadio&&M->FieldRadio->Message(FString(TEXT("DALE: "))+A.Text);
  else if(A.Who==TEXT("voice"))bSaid=M->FieldRadio&&M->FieldRadio->Message(A.Text);
  else if(A.Who==TEXT("cb"))bSaid=FS47FieldRadio::CbSay(M,{A.Text},TEXT("man"));
  else if(A.Who==TEXT("radio")&&M->Radio)
  {
   const TSharedPtr<FS47Station>* St=M->Radio->Stations.Find(TEXT("caliche"));
   bSaid=St&&St->IsValid()&&(*St)->CutIn({FS47RadioLine(A.Text,TEXT("host"),TEXT("the host"))},FMath::Fmod(F->Clock(),86400.))>0;
  }
  Log(bSaid?A.Who:A.Who+TEXT(": not heard (no set, or busy)"));
  break;
 }
 case ES47ScriptAct::Toast:
  if(M->UI)M->UI->Toast(A.Text,float(A.Secs>0?A.Secs:4));
  Log(TEXT(""));
  break;
 case ES47ScriptAct::Note:
 {
  if(!M->UI)break;
  FS47Doc D;D.Id=TEXT("script:")+Id;D.Title=A.Title;D.Kind=TEXT("hand");D.Page=A.Text;D.Transcript=A.Text;
  M->UI->Document(D);
  Log(A.Title);
  break;
 }
 case ES47ScriptAct::Evidence:
  if(bKeeps){F->RecordEvidence(A.Kind,A.Text);Log(A.Kind);}
  else{if(M->UI)M->UI->Toast(TEXT("(preview: the finding ")+A.Kind+TEXT(" is not kept in a real game)"),3);Log(TEXT("preview: not kept"));}
  break;
 case ES47ScriptAct::Look:
  if(!A.Point.bSet||!M->Player||(M->World&&M->World->bDriving)){Log(TEXT("refused: no point, or at the wheel"));break;}
  Look.Target=WorldOf(A.Point);Look.Total=Look.Left=FMath::Max(.2,A.Secs);Look.FromYaw=M->Player->Yaw;Look.FromPitch=M->Player->Pitch;Look.bLive=true;
  Log(TEXT(""));
  break;
 case ES47ScriptAct::Wait:
  Run.Hold=FMath::Max(0.,A.Secs);Run.bHoldLook=A.bFlag&&A.Point.bSet;Run.HoldAt=A.Point;
  Log(FString::Printf(TEXT("%.1f s%s"),A.Secs,Run.bHoldLook?TEXT(", or until looked at"):TEXT("")));
  break;
 case ES47ScriptAct::Shot:
 {
  // SHOT-BEAT-01: the eyes and the point in the area the player is in, each its Up over the ground there (as Look's point); the
  // player is never moved. A refused shot is one line and the timeline goes on.
  if(A.Path.IsEmpty()||!A.Point.bSet){UE_LOG(LogS47Script,Display,TEXT("S47: script shot skipped (no eye or no point)"));break;}
  FS47ShotSpec Spec;
  Spec.EyeA=WorldOf(A.Path[0]);Spec.EyeB=A.Path.Num()>1?WorldOf(A.Path[1]):Spec.EyeA;Spec.Target=WorldOf(A.Point);
  Spec.FovA=A.Fov;Spec.FovB=A.Fov2>0?A.Fov2:A.Fov;
  Spec.Secs=A.Secs;Spec.In=A.In;Spec.Out=A.Out;
  Spec.bMove=A.Path.Num()>1||Spec.FovB!=Spec.FovA;
  if(!Shot)Shot=MakeShared<FS47ShotCamera>(M);
  FString Why;
  if(Shot->Start(Spec,Why))Log(FString::Printf(TEXT("%d eye%s, %.0f to %.0f degrees, %.1f s"),A.Path.Num(),A.Path.Num()>1?TEXT("s"):TEXT(""),Spec.FovA,Spec.FovB,Spec.Secs));
  else UE_LOG(LogS47Script,Display,TEXT("S47: script shot skipped (%s)"),*Why);
  break;
 }
 default:break;
 }
}

void FS47ScriptPlayer::StepLook(double Dt)
{
 if(!Look.bLive||!M->Player)return;
 Look.Left-=Dt;
 const FVector D=Look.Target-EyeNow();
 const double ToYaw=FMath::Atan2(-D.X,-D.Z),ToPitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 const double K=FMath::SmoothStep(0.,1.,FMath::Clamp(1.-Look.Left/Look.Total,0.,1.));
 M->Player->Yaw=Look.FromYaw+FMath::UnwindRadians(ToYaw-Look.FromYaw)*K;
 M->Player->Pitch=FMath::Lerp(Look.FromPitch,ToPitch,K);
 if(Look.Left<=0)Look.bLive=false;
}

void FS47ScriptPlayer::StepFigure(FFigure& Fig,double Dt)
{
 if(!Bodies||!M->Web||!M->Player)return;
 FS47PaleBody& B=Bodies->Body[Fig.Body];
 const bool bCrab=Fig.Body==2;
 Fig.Clock+=Dt;
 FVector At=S47OpenLand::FromRoad(M->Web->Area(),Fig.RoadX,Fig.RoadZ);At.Y=Fig.Y;
 const FVector2D ToPlayer(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z);
 const double Face=FMath::Atan2(ToPlayer.Y,ToPlayer.X);
 double Gait=0;
 if(Fig.Seg<Fig.Path.Num())
 {
  const FVector To=WorldOf(Fig.Path[Fig.Seg]);
  const FVector2D Way(To.X-At.X,To.Z-At.Z);
  const double Left=Way.Size(),Step=Fig.Speed*Dt;
  if(Left<=Step){At.X=To.X;At.Z=To.Z;++Fig.Seg;}
  else
  {
   const double Want=FMath::Atan2(Way.Y,Way.X);
   Fig.Yaw+=FMath::UnwindRadians(Want-Fig.Yaw)*FMath::Min(1.,Dt*4.);
   At.X+=Way.X/Left*Step;At.Z+=Way.Y/Left*Step;
  }
  Gait=Fig.Speed>1.6?1.4:.85;
 }
 else
 {
  Fig.Yaw+=FMath::UnwindRadians(Face-Fig.Yaw)*FMath::Min(1.,Dt*1.5);
  Fig.Stand-=Dt;
 }
 // The ground under it, its own parts left out.
 FCollisionQueryParams Under(SCENE_QUERY_STAT(S47ScriptFigureFloor),true);
 for(const auto& Part:B.All)if(const auto* C=Part.Get())Under.AddIgnoredComponent(C);
 FHitResult Floor;
 const double Reach=Fig.bGround?3.:40.;
 if(M->Web->GetWorld()->LineTraceSingleByChannel(Floor,S47Three::ToUE(At.X,At.Y+Reach,At.Z),S47Three::ToUE(At.X,At.Y-Reach,At.Z),ECC_Visibility,Under))
 {
  const double G=S47Three::ToThree(Floor.ImpactPoint).Y;
  At.Y=Fig.bGround?FMath::FInterpTo(At.Y,G,Dt,12.):G;Fig.bGround=true;
 }
 Fig.Y=At.Y;
 double RX=0,RZ=0;
 if(S47OpenLand::RoadOf(M->Web->Area(),At.X,At.Z,RX,RZ)){Fig.RoadX=RX;Fig.RoadZ=RZ;}
 // Gone when looked at, if it should be: a moment in view, then nothing.
 if(Fig.bVanish&&FVector2D(ToPlayer).Size()<150&&LookingAt(At+FVector(0,1.2,0),10.))
 {
  Fig.Seen+=Dt;
  if(Fig.Seen>=S47ScriptPlayerLocal::SeenSeconds){Fig.Stand=0;Fig.Seg=Fig.Path.Num();}
 }
 const bool bShow=Fig.Seg<Fig.Path.Num()||Fig.Stand>0;
 if(bCrab)B.ApplyCrab(bShow,At,Fig.Yaw,Face,Fig.Clock,FMath::Clamp(Fig.Clock/2.5,0.,1.),false);
 else B.Apply(bShow,At,Fig.Yaw,Face,Gait,Fig.Clock*3,Fig.Clock,Gait>0?FS47PaleBody::EPose::Walk:FS47PaleBody::EPose::Listen);
 if(!bShow){Fig.bLive=false;Bodies->bInUse[Fig.Body]=false;}
}
