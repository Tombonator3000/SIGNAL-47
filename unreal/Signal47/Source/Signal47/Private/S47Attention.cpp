#include "S47Attention.h"
#include "S47DevEvents.h"
#include "S47EventScript.h"
#include "S47FieldCamera.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightTiers.h"
#include "S47Omen.h"
#include "S47SaroSave.h"
#include "S47ScriptPlayer.h"
#include "S47Seen.h"
#include "S47SkyVisitors.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Attention,Log,All);

// N1110 (N1110-1, N1110-2): see S47Attention.h. The scripts it plays are Content/Director/Events/attention_*.json.

namespace S47AttentionLocal
{
 const TCHAR* const SabNames[]={TEXT("none"),TEXT("relay"),TEXT("mains"),TEXT("lamp"),TEXT("truck")};
 const TCHAR* const HintRule=TEXT("attention_hint_dale");
 const TCHAR* const HintVisit=TEXT("attention_hint_visit");
 const TCHAR* const VisitId=TEXT("attention_visit");
 const TCHAR* const VisitHeld=TEXT("attention_visit_held");
 const TCHAR* const VisitEscaped=TEXT("attention_visit_escaped");
 // A sabotage on the mains: a short sag, then out long enough that the standby generator matters.
 constexpr double SabSag=20,SabOut=240;
 // Marks are walked into within this, and left beyond the second (so a step out and back is not a return).
 constexpr double MarksIn=15,MarksOut=40;
 FS47FreeformSession* Session(US47Main* M){return M?M->Freeform.Get():nullptr;}
 FS47Flashlight* Lamp(US47Main* M){return M&&M->Night?M->Night->Flashlight.Get():nullptr;}
}

// ---------- the rule ----------

int32 FS47Attention::TierOf(double Score){return Score>=VisitAt?3:Score>=SabotageAt?2:Score>=WarnAt?1:0;}
double FS47Attention::Fall(double Score,double GameSeconds){return FMath::Clamp(Score-FallPerDay*FMath::Max(0.,GameSeconds)/86400.,0.,Top);}
double FS47Attention::StareGain(double HeldFor,double Dt,bool bCamera)
{
 const double Grace=bCamera?CameraGrace:StareGrace;
 return HeldFor<=Grace||Dt<=0?0.:(bCamera?CameraPerSec:StarePerSec)*FMath::Min(Dt,HeldFor-Grace);
}
double FS47Attention::ReturnGain(int32 Times){return Times<=1?0.:ReturnAgain*FMath::Min(Times-1,2);}
double FS47Attention::PhotoGain(double Quality){return PhotoBase+PhotoClose*FMath::Clamp(Quality,0.,1.);}
bool FS47Attention::WarnHours(double Hour){return Hour>=18||Hour<5.5;}
bool FS47Attention::SabotageHours(double Hour){return Hour>=21||Hour<5;}
bool FS47Attention::VisitHours(double Hour,int32 Night,bool bEarly)
{
 const double H=S47NightTiers::NightHour(Hour);
 return H>=(bEarly?24.:S47NightTiers::FullHour(Night))&&H<29.5;
}
bool FS47Attention::VisitMay(int32 Day,int32 Night,int32 LastVisitNight){return Day>=FirstDay&&(LastVisitNight<=0||Night>=LastVisitNight+2);}
FS47Attention::EOutcome FS47Attention::OutcomeFor(bool bAtSaro,bool bInside,bool bPower)
{
 return !bAtSaro?EOutcome::Gone:bInside&&bPower?EOutcome::Held:bInside?EOutcome::Flood:EOutcome::Over;
}
double FS47Attention::AfterVisit(EOutcome O,bool bTaken,double Score)
{
 // Nobody home: it is still about. Held off with the lights: it leaves them be a while. Got away from the light, less so.
 if(O==EOutcome::Gone)return FMath::Clamp(Score-15,0.,Top);
 if(O==EOutcome::Held)return FMath::Min(Score,40.);
 return FMath::Min(Score,bTaken?15.:45.);
}
const TCHAR* FS47Attention::SabName(ESab K){return S47AttentionLocal::SabNames[FMath::Clamp(int32(K),0,4)];}
FS47Attention::ESab FS47Attention::SabFrom(const FString& Name)
{
 for(int32 I=1;I<5;++I)if(Name==S47AttentionLocal::SabNames[I])return ESab(I);
 return ESab::None;
}
FS47Attention::ESab FS47Attention::PickSab(bool bAtSaro,int32 Turn,TFunctionRef<bool(ESab)> Can)
{
 static const ESab Here[]={ESab::Relay,ESab::Mains,ESab::Lamp},Away[]={ESab::Truck,ESab::Lamp};
 const ESab* L=bAtSaro?Here:Away;const int32 N=bAtSaro?3:2;
 for(int32 I=0;I<N;++I){const ESab K=L[(FMath::Max(0,Turn)+I)%N];if(Can(K))return K;}
 return ESab::None;
}
const TArray<FString>& FS47Attention::WarnScripts()
{
 static const TArray<FString> L={TEXT("attention_warn_tv"),TEXT("attention_warn_call"),TEXT("attention_warn_dog"),TEXT("attention_warn_lights"),TEXT("attention_warn_thought")};
 return L;
}

// ---------- the session's ----------

void FS47Attention::Begin(const S47J::FObj& Saved)
{
 Value=FMath::Clamp(S47J::Num(Saved,TEXT("score")),0.,Top);LastClock=-1;
 WarnNight=S47J::Int(Saved,TEXT("warnNight"));WarnCount=S47J::Int(Saved,TEXT("warns"));
 SabNight=S47J::Int(Saved,TEXT("sabNight"));SabCount=S47J::Int(Saved,TEXT("sabs"));Sab=SabFrom(S47J::Str(Saved,TEXT("sab")));
 VisitNight=S47J::Int(Saved,TEXT("visitNight"));VisitCount=S47J::Int(Saved,TEXT("visits"));
 Tracked.Reset();Claims.Reset();
 for(const FString& Id:S47J::Strs(Saved,TEXT("tracked")))Tracked.Add(Id);
 for(const FString& Id:S47J::Strs(Saved,TEXT("claims")))Claims.Add(Id);
 // An older save, or one from before anything counted: what is already recorded is not counted again on the first step.
 bBaseline=S47J::Has(Saved,TEXT("score"));
 bDev=bTruckArmed=false;WarnWait=SabWait=VisitWait=StareFor=VisitT=0;ChaseDist=-1;Visit=EVisit::None;Who=0;TakenBefore=0;
 Returns.Reset();NearMarks.Reset();ReturnNight=0;
}

void FS47Attention::Stop()
{
 // A visit under way is not kept: its sky cut and its light are the session's and FS47Taken's, which stop with it.
 Visit=EVisit::None;VisitT=0;bTruckArmed=false;
}

S47J::FObj FS47Attention::Save() const
{
 const S47J::FObj O=S47J::New();
 if(Value<=0&&WarnCount==0&&SabCount==0&&VisitCount==0&&Sab==ESab::None)return O;
 S47J::Set(O,TEXT("score"),FMath::RoundToDouble(Value*100.)/100.);
 if(WarnCount){S47J::Set(O,TEXT("warnNight"),WarnNight);S47J::Set(O,TEXT("warns"),WarnCount);}
 if(SabCount){S47J::Set(O,TEXT("sabNight"),SabNight);S47J::Set(O,TEXT("sabs"),SabCount);}
 if(Sab!=ESab::None&&Sab!=ESab::Truck)S47J::Set(O,TEXT("sab"),SabName(Sab)); // the truck's is not saved (S47World's bKeyStall is not)
 if(VisitCount){S47J::Set(O,TEXT("visitNight"),VisitNight);S47J::Set(O,TEXT("visits"),VisitCount);}
 TArray<FString> T=Tracked.Array();T.Sort();if(T.Num())S47J::Set(O,TEXT("tracked"),T);
 TArray<FString> C=Claims.Array();C.Sort();if(C.Num())S47J::Set(O,TEXT("claims"),C);
 return O;
}

bool FS47Attention::Live() const
{
 const FS47FreeformSession* F=S47AttentionLocal::Session(M);
 return F&&F->Active()&&(F->RulesLive()||bDev);
}

bool FS47Attention::AtSaro() const
{
 return M&&M->Web&&M->Web->Area()==TEXT("saro")&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving;
}

void FS47Attention::Add(double Points,const TCHAR* Why)
{
 if(!(Points>0))return;
 const int32 Was=TierOf(Value);
 Value=FMath::Clamp(Value+Points,0.,Top);
 if(Points>=1||TierOf(Value)!=Was)UE_LOG(LogS47Attention,Display,TEXT("S47: attention +%.1f (%s), now %.1f"),Points,Why,Value);
}

FString FS47Attention::Status() const
{
 static const TCHAR* const Tiers[]={TEXT("calm"),TEXT("warnings"),TEXT("sabotage"),TEXT("a visit")};
 return FString::Printf(TEXT("attention %.0f (%s); sabotage now: %s; %s; warned %d, sabotaged %d, visits %d"),Value,Tiers[TierOf(Value)],SabName(Sab),Visit==EVisit::None?TEXT("no visit under way"):Visit==EVisit::Coming?TEXT("a visit coming"):TEXT("the light is looking"),WarnCount,SabCount,VisitCount);
}

void FS47Attention::TakeBaseline()
{
 const FS47FreeformSession* F=S47AttentionLocal::Session(M);
 if(!F)return;
 for(const auto& J:F->Jobs())if(J.bAnomaly&&uint8(J.Stage)>=uint8(FS47FreeformSession::EJobStage::Recorded))Tracked.Add(J.Id);
 if(F->Director)for(const auto& C:F->Director->PhotoClaims())Claims.Add(C.Id);
}

void FS47Attention::Step(double Dt)
{
 FS47FreeformSession* F=S47AttentionLocal::Session(M);
 if(!F||!F->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 if(!bLinesAdded)AddDevLines();
 // It falls by the game's clock: a rest or a wait falls it as much as the hours it skips.
 const double Clock=F->Clock();
 if(LastClock>=0&&Clock>LastClock)Value=Fall(Value,Clock-LastClock);
 LastClock=Clock;
 if(!Live())return;
 if(!bBaseline){bBaseline=true;TakeBaseline();}
 const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 const int32 Night=FS47FreeformSession::NightOf(Clock),Day=F->Day();
 if(Visit!=EVisit::None){StepVisit(Dt);return;}
 Gains(Dt);
 StepSabotage(Dt,Hour,Night,Day);
 StepWarn(Dt,Hour,Night);
 StepVisitDraw(Dt,Hour,Night,Day);
}

void FS47Attention::Gains(double Dt)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F)return;
 // The card and the photographs, wherever the player is.
 for(const auto& J:F->Jobs())
 {
  if(!J.bAnomaly||uint8(J.Stage)<uint8(FS47FreeformSession::EJobStage::Recorded)||Tracked.Contains(J.Id))continue;
  const bool bAgain=F->Jobs().ContainsByPredicate([&J,this](const FS47FreeformSession::FJob& K){return K.Id!=J.Id&&K.bAnomaly&&K.Source==J.Source&&Tracked.Contains(K.Id)&&FMath::Abs(K.Day-J.Day)<=RepeatDays;});
  Tracked.Add(J.Id);
  Add(TrackRecorded+(bAgain?TrackAgain:0.),bAgain?TEXT("the same strange source recorded again"):TEXT("a strange source recorded"));
 }
 if(const FS47FreeformSession::FJob* Now=F->CurrentJob())if(Now->bAnomaly&&Now->Stage==FS47FreeformSession::EJobStage::Integrating&&F->HasPowerForReactiveLight())Add(TrackPerSec*Dt,TEXT("the array held on it"));
 if(F->Director)for(const auto& C:F->Director->PhotoClaims())if(!Claims.Contains(C.Id)){Claims.Add(C.Id);Add(PhotoGain(C.Quality),TEXT("a photograph of it"));}
 // Out there, on foot: what the player does about what is near (FS47Omen's sources, the same the gear reads).
 if(!M->Player||!M->Omen||(M->World&&M->World->bDriving)){StareFor=0;ChaseDist=-1;return;}
 const FS47Cam Cam=FS47Cam::Of(M->Player.Get());
 const bool bCamera=M->FieldCamera&&M->FieldCamera->bHave&&M->FieldCamera->bRaised;
 const FS47Flashlight* L=Lamp(M);
 const bool bLampOn=L&&L->bHave&&L->bOn&&!L->Dead();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(ReturnNight!=Night){ReturnNight=Night;Returns.Reset();NearMarks.Reset();}
 const FString Area=M->Web?M->Web->Area():FString();
 bool bStare=false,bLit=false;double Nearest=-1;
 TSet<FString> NearNow;
 for(const FS47OmenSource& S:M->Omen->Sources())
 {
  // Not what this plays itself, not being taken, not a face at a window or a door: those find the player, not the other way.
  if(S.Id.StartsWith(TEXT("event:"))||S.Id.StartsWith(TEXT("script:"))||S.Id==TEXT("taken")||S.Id==TEXT("door")||S.Id==TEXT("peek")||S.Id==TEXT("dev"))continue;
  const double Dist=FVector::Dist(Feet,S.At);
  if(S.bResidual)
  {
   const FString Key=FString::Printf(TEXT("%s@%d,%d"),*Area,FMath::RoundToInt(S.At.X/10.),FMath::RoundToInt(S.At.Z/10.));
   if(Dist<=MarksIn||(Dist<=MarksOut&&NearMarks.Contains(Key)))
   {
    NearNow.Add(Key);
    if(!NearMarks.Contains(Key)){int32& Times=Returns.FindOrAdd(Key);++Times;Add(ReturnGain(Times),TEXT("back in the same fresh marks"));}
   }
   continue;
  }
  const bool bBody=S.Id==TEXT("visit")||S.Id.StartsWith(TEXT("body:"));
  if(bBody)
  {
   if(bLampOn&&Dist<25&&S47Facing(Cam,S.At+FVector(0,1,0),25,10))bLit=true;
   if(Dist<40&&(Nearest<0||Dist<Nearest))Nearest=Dist;
  }
  if(!S.bComing&&S47Facing(Cam,S.At,3000,bCamera?10:14))bStare=true;
 }
 NearMarks=MoveTemp(NearNow);
 StareFor=bStare?StareFor+Dt:0;
 Add(StareGain(StareFor,Dt,bCamera),bCamera?TEXT("a long look through the camera"):TEXT("a long look"));
 if(bLit)Add(LampPerSec*Dt,TEXT("the flashlight on one of them"));
 if(Nearest>=0&&ChaseDist>=0&&(ChaseDist-Nearest)/Dt>ChaseMps)Add(ChasePerSec*Dt,TEXT("closing on one of them"));
 ChaseDist=Nearest;
}

bool FS47Attention::Gated(const FString& Id) const
{
 FString Why;
 const bool bOk=S47EventScript::Allows(M,Id,Why);
 if(!bOk)UE_LOG(LogS47Attention,Verbose,TEXT("S47: attention: %s held back: %s"),*Id,*Why);
 return bOk;
}

bool FS47Attention::Play(const FString& Id) const
{
 FS47EventCtx Ctx;
 const FS47FreeformSession* F=S47AttentionLocal::Session(M);
 // The array's cabinet with the fault, for a sound placed there.
 if(F&&F->Field&&F->Field->HasArrayFault())Ctx.Points.Add(TEXT("cabinet"),F->Field->ServicePosition()+FVector(0,1,0));
 const bool bPlayed=S47EventScript::Play(M,Id,Ctx);
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention plays %s%s"),*Id,bPlayed?TEXT(""):TEXT(": no script, a plain line instead"));
 return bPlayed;
}

void FS47Attention::StepWarn(double Dt,double Hour,int32 Night)
{
 if(Value<WarnAt||WarnNight==Night||!WarnHours(Hour)){WarnWait=0;return;}
 if((WarnWait+=Dt)<WarnDelay)return;
 WarnWait=0;
 const TArray<FString>& All=WarnScripts();
 for(int32 K=0;K<All.Num();++K)
 {
  const FString& Id=All[(WarnCount+K)%All.Num()];
  if(!Gated(Id))continue;
  // Lights over the station: a slow formation crossing the sky over SARO, as a director script puts one up.
  if(Id==TEXT("attention_warn_lights")&&M->SkyVisitors&&AtSaro())M->SkyVisitors->DevFormation(ES47SkyForm::Circle,0,900.,450.);
  if(!Play(Id)&&M->UI)M->UI->Thought(TEXT("I can't shake the feeling something is looking back."),4.5f);
  if(WarnCount==0)Play(S47AttentionLocal::HintRule);
  WarnNight=Night;++WarnCount;
  return;
 }
}

bool FS47Attention::CanSab(ESab K) const
{
 using namespace S47AttentionLocal;
 const FS47FreeformSession* F=Session(M);
 if(!F)return false;
 const FS47Flashlight* L=Lamp(M);
 switch(K)
 {
 case ESab::Relay:return F->Field&&!F->Field->HasArrayFault()&&!F->PowerFault();
 case ESab::Mains:return !F->SkyCutLive()&&!F->PowerFault();
 case ESab::Lamp:return L&&L->bHave&&L->bDrains&&L->BatteryLevel()>.3;
 case ESab::Truck:return M->World&&!M->World->bDriving&&!M->World->bKeyStall&&!M->World->Trouble.WantsKey();
 default:return false;
 }
}

bool FS47Attention::SabOver() const
{
 using namespace S47AttentionLocal;
 const FS47FreeformSession* F=Session(M);
 if(!F)return true;
 const FS47Flashlight* L=Lamp(M);
 switch(Sab)
 {
 case ESab::Relay:return !F->Field||!F->Field->HasArrayFault();
 case ESab::Mains:return !F->SkyCutLive();
 case ESab::Lamp:return !L||!L->bHave||L->BatteryLevel()>=.5;
 case ESab::Truck:return !bTruckArmed&&!(M->World&&M->World->Trouble.WantsKey());
 default:return true;
 }
}

bool FS47Attention::DoSabotage(bool bForced)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||Sab!=ESab::None)return false;
 const ESab K=PickSab(AtSaro(),SabCount,[this](ESab X){return CanSab(X);});
 bool bDone=false;
 switch(K)
 {
 case ESab::Relay:bDone=F->Field&&F->Field->TriggerServiceFault(false);break;
 case ESab::Mains:F->SkyCutBegin(SabSag,SabOut);bDone=F->SkyCutLive();break;
 case ESab::Lamp:if(FS47Flashlight* L=Lamp(M)){L->SetBattery(FMath::Min(L->BatteryLevel(),.06));bDone=true;}break;
 case ESab::Truck:M->World->bKeyStall=true;bTruckArmed=true;bDone=true;break;
 default:break;
 }
 if(!bDone)return false;
 Sab=K;++SabCount;
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention %.0f: sabotage, %s%s"),Value,SabName(K),bForced?TEXT(" (F10)"):TEXT(""));
 // The truck's trace waits for the player to be at the wheel (StepSabotage).
 if(K!=ESab::Truck)Play(FString(TEXT("attention_sab_"))+SabName(K));
 if(SabCount==1)Play(HintVisit);
 return true;
}

void FS47Attention::StepSabotage(double Dt,double Hour,int32 Night,int32 Day)
{
 // The truck that wants the key: its trace when the player is at the wheel and it does not start.
 if(Sab==ESab::Truck&&bTruckArmed&&M->World&&!M->World->bKeyStall)
 {
  bTruckArmed=false;
  if(M->World->bDriving&&M->World->Trouble.WantsKey())Play(TEXT("attention_sab_truck"));
 }
 if(Sab!=ESab::None&&SabOver()){UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the %s is put right"),SabName(Sab));Sab=ESab::None;}
 if(Value<SabotageAt||Day<FirstDay||SabNight==Night||Sab!=ESab::None||!SabotageHours(Hour)){SabWait=0;return;}
 FString Why;
 if(FS47ScriptPlayer::BigEvent(M,Why))return;
 if((SabWait+=Dt)<SabotageDelay)return;
 SabWait=0;
 if(DoSabotage(false))SabNight=Night;
}

void FS47Attention::StepVisitDraw(double Dt,double Hour,int32 Night,int32 Day)
{
 using namespace S47AttentionLocal;
 const FS47FreeformSession* F=Session(M);
 if(!F||Value<VisitAt||!VisitMay(Day,Night,VisitNight)||!AtSaro()){VisitWait=0;return;}
 const FS47EventScript* S=S47EventScript::Find(VisitId);
 if(!VisitHours(Hour,Night,S&&S->Gates.bEarly)){VisitWait=0;return;}
 FString Why;
 if(FS47ScriptPlayer::BigEvent(M,Why)||(F->Taken&&F->Taken->Busy())||!Gated(VisitId))return;
 if((VisitWait+=Dt)<VisitDelay)return;
 VisitWait=0;
 BeginVisit();
}

void FS47Attention::BeginVisit()
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||Visit!=EVisit::None)return;
 Visit=EVisit::Coming;VisitT=0;VisitNight=FS47FreeformSession::NightOf(F->Clock());TakenBefore=F->TimesTaken();Who=Value>=95?2:0;
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention %.0f: a visit at SARO"),Value);
 if(!Play(VisitId)&&M->UI)M->UI->Thought(TEXT("Something is right over the station."),4.5f);
 // It comes as the mains go: the sag's own lines point at the standby generator (FS47FreeformSession::StepSky).
 F->SkyCutBegin(VisitSag,VisitOut);
}

void FS47Attention::StepVisit(double Dt)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F)return;
 VisitT+=Dt;
 if(Visit==EVisit::Coming)
 {
  if(VisitT<VisitLook)return;
  const bool bAt=AtSaro();
  const EOutcome O=OutcomeFor(bAt,bAt&&S47SaroSave::InStation(M),F->HasPowerForReactiveLight());
  if(O==EOutcome::Gone){EndVisit(O,false);return;}
  if(O==EOutcome::Held){Play(VisitHeld);EndVisit(O,false);return;}
  const ES47Taker T=Who==2?ES47Taker::Tall:ES47Taker::Roswell;
  bool bBegun=false;
  if(F->Taken&&M->Player)
   bBegun=O==EOutcome::Flood?F->Taken->Begin(T,TEXT("the station")):F->Taken->BeginOver(T,TEXT("the yard at SARO"),FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z));
  // The night's rules let no second danger in, or the light has no way to them: it holds, and goes.
  if(!bBegun){Play(VisitHeld);EndVisit(EOutcome::Held,false);return;}
  Visit=EVisit::Taking;
  return;
 }
 if(F->Taken&&F->Taken->Busy()&&VisitT<600)return;
 const bool bTaken=F->TimesTaken()>TakenBefore;
 if(!bTaken)Play(VisitEscaped);
 EndVisit(EOutcome::Flood,bTaken);
}

void FS47Attention::EndVisit(EOutcome O,bool bTaken)
{
 Value=AfterVisit(O,bTaken,Value);Visit=EVisit::None;VisitT=0;++VisitCount;
 static const TCHAR* const Names[]={TEXT("nobody at the station"),TEXT("held off with the lights on"),TEXT("the light through the windows"),TEXT("the light over the yard")};
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the visit is over (%s%s), now %.0f"),Names[int32(O)],bTaken?TEXT(", the player went with it"):TEXT(""),Value);
}

void FS47Attention::AddDevLines()
{
 bLinesAdded=true;
 if(!M||!M->DevEvents)return;
 static const TCHAR* const Group=TEXT("Attention (N1110)");
 US47Main* Main=M;
 const auto Have=[Main](const FString& Label){return Main->DevEvents->Events().ContainsByPredicate([&Label](const FS47DevEvents::FEvent& E){return E.Group==Group&&E.Label==Label;});};
 const auto Line=[&](const TCHAR* Label,const TCHAR* Note,TFunction<FString(FS47Attention&)> Fn)
 {
  if(Have(Label))return;
  Main->DevEvents->AddFreeform(Group,Label,Note,[Main,Fn]
  {
   FS47Attention* A=Main->Freeform&&Main->Freeform->Attention?Main->Freeform->Attention.Get():nullptr;
   const FString Said=A?Fn(*A):FString(TEXT("Freeform only"));
   if(Main->UI)Main->UI->Toast(TEXT("Developer event: ")+Said,5);
  });
 };
 Line(TEXT("Where the attention stands"),TEXT("the hidden number, its tier, the sabotage and the visit"),[](FS47Attention& A){return A.Status();});
 Line(TEXT("Attention: calm (0)"),TEXT("nothing more comes of it"),[](FS47Attention& A){A.bDev=true;A.Value=0;return A.Status();});
 Line(TEXT("Attention: warnings (30)"),TEXT("a warning in about a minute, in its hours, whatever was warned tonight"),[](FS47Attention& A){A.bDev=true;A.Value=FMath::Max(A.Value,30.);A.WarnNight=0;A.WarnWait=WarnDelay;return A.Status();});
 Line(TEXT("Attention: sabotage now"),TEXT("one piece of equipment, now, past the hour (one at a time)"),[](FS47Attention& A){A.bDev=true;A.Value=FMath::Max(A.Value,55.);const bool bDone=A.DoSabotage(true);if(bDone&&A.M&&A.M->Freeform)A.SabNight=FS47FreeformSession::NightOf(A.M->Freeform->Clock());return bDone?A.Status():FString(TEXT("nothing to sabotage here now, or one is still out: "))+A.Status();});
 Line(TEXT("Attention: a visit (80)"),TEXT("a visit at SARO in half a minute once its hours and the rules allow"),[](FS47Attention& A){A.bDev=true;A.Value=FMath::Max(A.Value,80.);A.VisitNight=0;return A.Status();});
 Line(TEXT("Attention: a visit now (at SARO)"),TEXT("past the hour and the dice; at SARO, not driving"),[](FS47Attention& A){A.bDev=true;A.Value=FMath::Max(A.Value,80.);if(!A.AtSaro())return FString(TEXT("at SARO only"));A.BeginVisit();return A.Status();});
}

// ---------- the self-test ----------

bool FS47Attention::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s attention: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 Check(TierOf(0)==0&&TierOf(WarnAt-.1)==0&&TierOf(WarnAt)==1&&TierOf(SabotageAt)==2&&TierOf(VisitAt)==3&&TierOf(Top)==3&&WarnAt<SabotageAt&&SabotageAt<VisitAt&&VisitAt<Top,TEXT("warnings, then sabotage, then a visit, in that order"));
 // The curve: it falls slowly, over days.
 Check(FMath::IsNearlyEqual(Fall(80,86400.),80.-FallPerDay)&&Fall(10,2*86400.)==0&&Fall(50,-5)==50,TEXT("it falls by itself, FallPerDay a game day, never below nothing"));
 Check(Fall(VisitAt,3*86400.)>WarnAt&&Fall(Top,7*86400.)<WarnAt,TEXT("from a visit's level it still warns three days on, and a quiet week is calm again"));
 // A look costs nothing; a long one does, more through the camera.
 const auto Look=[](double Seconds,bool bCamera){double Sum=0,Held=0;for(double T=0;T<Seconds-1e-9;T+=.05){Held+=.05;Sum+=StareGain(Held,.05,bCamera);}return Sum;};
 Check(Look(3,false)==0&&Look(1.5,true)==0,TEXT("a glance, even through the camera, adds nothing"));
 Check(FMath::IsNearlyEqual(Look(30,false),StarePerSec*(30-StareGrace),.01)&&FMath::IsNearlyEqual(Look(30,true),CameraPerSec*(30-CameraGrace),.01)&&Look(30,true)>2*Look(30,false),TEXT("half a minute held on a light adds, through the camera more than twice as much"));
 Check(ReturnGain(1)==0&&ReturnGain(2)==ReturnAgain&&ReturnGain(3)==2*ReturnAgain&&ReturnGain(6)==2*ReturnAgain,TEXT("the first walk into fresh marks is the job; going back again is not"));
 Check(PhotoGain(0)==PhotoBase&&PhotoGain(1)==PhotoBase+PhotoClose&&PhotoGain(5)==PhotoGain(1),TEXT("a photograph adds, more when it fills the print"));
 // An ordinary night of work stays calm; a night of bothering them comes to a visit.
 {
  const double Work=2*TrackRecorded+2*PhotoGain(.5)+4*Look(3,true)+Look(10,false)+ReturnGain(1);
  Check(Work<WarnAt,FString::Printf(TEXT("an ordinary night (two strange cards, two photographs, short looks, one walk into marks) stays calm: %.1f"),Work));
  const double Pester=TrackRecorded+2*(TrackRecorded+TrackAgain)+Look(60,true)+10*LampPerSec+2*PhotoGain(.95)+ReturnGain(2)+ReturnGain(3);
  Check(Pester>=VisitAt,FString::Printf(TEXT("a night of bothering them (the same source three times, a minute through the camera, the lamp on one, close photographs, back in the marks) comes to a visit: %.1f"),Pester));
  const double Watch=Look(90,false);
  Check(Watch>=WarnAt-10&&Watch<SabotageAt,FString::Printf(TEXT("a long watch with the eyes alone comes near the warnings, not to sabotage: %.1f"),Watch));
 }
 // The hours of each tier.
 Check(WarnHours(18.5)&&WarnHours(4)&&!WarnHours(12)&&!WarnHours(6),TEXT("warnings from 18:00 to 05:30"));
 Check(SabotageHours(22)&&SabotageHours(3)&&!SabotageHours(20)&&!SabotageHours(5.5),TEXT("sabotage from 21:00 to 05:00"));
 Check(!VisitHours(23,3,false)&&!VisitHours(1.5,3,false)&&VisitHours(2.5,3,false)&&VisitHours(5,3,false)&&!VisitHours(6,3,false),TEXT("a visit only in the full tier, after two, before half past five"));
 Check(VisitHours(1,3,true)&&!VisitHours(23,3,true)&&VisitHours(1.5,S47NightTiers::LateFromNight,false),TEXT("the authored escalation from midnight, and the late nights' earlier full tier"));
 Check(!VisitMay(1,1,0)&&VisitMay(2,2,0)&&!VisitMay(3,3,2)&&VisitMay(4,4,2),TEXT("no visit on the first day, at most one in two nights"));
 // The visit: what saves the player is what the game already has.
 Check(OutcomeFor(false,false,false)==EOutcome::Gone&&OutcomeFor(true,true,true)==EOutcome::Held&&OutcomeFor(true,true,false)==EOutcome::Flood&&OutcomeFor(true,false,true)==EOutcome::Over,TEXT("away it finds nobody; inside with power it holds; inside in the dark the light looks in; in the yard it stands over them"));
 Check(AfterVisit(EOutcome::Held,false,90)<VisitAt&&AfterVisit(EOutcome::Flood,true,90)<AfterVisit(EOutcome::Held,false,90)&&AfterVisit(EOutcome::Flood,false,90)<VisitAt&&AfterVisit(EOutcome::Gone,false,80)<80&&AfterVisit(EOutcome::Held,false,20)==20,TEXT("after a visit it falls below a visit, lowest when the player went with it, never up"));
 // Sabotage: one kind at a time, in turn, only what can be done.
 {
  const auto All=[](ESab){return true;};
  const auto NoRelay=[](ESab K){return K!=ESab::Relay;};
  const auto Nothing=[](ESab){return false;};
  Check(PickSab(true,0,All)==ESab::Relay&&PickSab(true,1,All)==ESab::Mains&&PickSab(true,2,All)==ESab::Lamp&&PickSab(true,3,All)==ESab::Relay,TEXT("at SARO: the relay, the mains, the lamp, in turn"));
  Check(PickSab(false,0,All)==ESab::Truck&&PickSab(false,1,All)==ESab::Lamp&&PickSab(true,0,NoRelay)==ESab::Mains&&PickSab(true,0,Nothing)==ESab::None,TEXT("away: the truck, the lamp; what cannot be done is passed over, and nothing when nothing can"));
  bool bNoTruck=true;for(int32 T=0;T<6;++T)bNoTruck&=PickSab(true,T,All)!=ESab::Truck;
  Check(bNoTruck,TEXT("the truck is never the station's"));
  bool bNames=true;for(ESab K:{ESab::Relay,ESab::Mains,ESab::Lamp,ESab::Truck})bNames&=SabFrom(SabName(K))==K;
  Check(bNames&&SabFrom(TEXT("printer"))==ESab::None,TEXT("each kind keeps its name in the save"));
 }
 // The save: optional keys, and the same back.
 {
  FS47Attention A(nullptr);A.Begin(S47J::FObj());
  Check(A.Value==0&&A.Save()->Values.Num()==0&&!A.bBaseline,TEXT("an older save starts at nothing, and nothing is written while nothing has happened"));
  A.Value=63.25;A.WarnCount=2;A.WarnNight=5;A.SabCount=1;A.SabNight=5;A.Sab=ESab::Relay;A.VisitCount=1;A.VisitNight=4;A.Tracked.Add(TEXT("j-3-1"));A.Claims.Add(TEXT("strange.body.n5"));
  FS47Attention B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(FMath::IsNearlyEqual(B.Value,63.25)&&B.WarnCount==2&&B.WarnNight==5&&B.SabCount==1&&B.Sab==ESab::Relay&&B.VisitCount==1&&B.VisitNight==4&&B.Tracked.Contains(TEXT("j-3-1"))&&B.Claims.Contains(TEXT("strange.body.n5"))&&B.bBaseline,TEXT("what it has counted and done comes back from the save"));
  A.Sab=ESab::Truck;FS47Attention C(nullptr);C.Begin(A.Save());
  Check(C.Sab==ESab::None,TEXT("the truck's sabotage is not saved, as the stall it arms is not"));
 }
 // The scripts it plays: there, and clean (the spoiler rules and the word never said to the player).
 {
  TArray<FString> Ids=WarnScripts();
  Ids.Append({S47AttentionLocal::HintRule,S47AttentionLocal::HintVisit,S47AttentionLocal::VisitId,S47AttentionLocal::VisitHeld,S47AttentionLocal::VisitEscaped});
  for(ESab K:{ESab::Relay,ESab::Mains,ESab::Lamp,ESab::Truck})Ids.Add(FString(TEXT("attention_sab_"))+SabName(K));
  int32 Found=0;bool bClean=true;FString First;
  for(const FString& Id:Ids)
  {
   const FS47EventScript* S=S47EventScript::Find(Id);if(!S)continue;
   ++Found;const TArray<FString> E=S47EventScript::Lint(*S);
   if(E.Num()){bClean=false;if(First.IsEmpty())First=Id+TEXT(": ")+E[0];}
  }
  if(Found==0)Lines.Add(TEXT("NOTE attention: no attention_* event scripts beside this build: plain lines play instead"));
  else Check(bClean&&Found==Ids.Num(),FString::Printf(TEXT("all %d of its event scripts are there and clean%s"),Ids.Num(),First.IsEmpty()?TEXT(""):*(TEXT(": ")+First)));
  const FS47EventScript* V=S47EventScript::Find(S47AttentionLocal::VisitId);
  if(V)Check(V->Gates.bTier&&V->Gates.Tier==ES47Allowed::Full&&!V->Gates.bEarly,TEXT("the visit's script keeps to the full tier (no early escalation as shipped)"));
 }
 return OK;
}
