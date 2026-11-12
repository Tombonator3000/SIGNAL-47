#include "S47Attention.h"
#include "S47DevEvents.h"
#include "S47DoorLoop.h"
#include "S47EventPlaces.h"
#include "S47EventScript.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47NightTiers.h"
#include "S47Omen.h"
#include "S47Rhythm.h"
#include "S47SaroSave.h"
#include "S47ScriptPlayer.h"
#include "S47Seen.h"
#include "S47SkyVisitors.h"
#include "S47StoryDirector.h"
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
 const TCHAR* const HintNeutral=TEXT("attention_hint_dale_neutral"); // N1110 (pace, critic 2): the day-3 hint, with no lights named
 const TCHAR* const HintVisit=TEXT("attention_hint_visit");
 const TCHAR* const TellId=TEXT("attention_tell"); // N1110 (pace): a one-second dip of the ambience
 const TCHAR* const HintKey=TEXT("attention_rule"); // the field record Dale's word keeps (attention_hint_dale.json)
 const TCHAR* const VisitId=TEXT("attention_visit");
 const TCHAR* const VisitHeld=TEXT("attention_visit_held");
 const TCHAR* const VisitEscaped=TEXT("attention_visit_escaped");
 const TCHAR* const FeederTrace=TEXT("attention_sab_feeder"); // POWER-CHOICE-20261112: at the panel, with the lockout the sabotage left
 const TCHAR* const FeederPlace=TEXT("feeder"); // its place on the list (FS47EventPlaces, PT-FEEDER-20261109)
 // A sabotage on the mains: a short sag, then out long enough that the standby generator matters.
 constexpr double SabSag=20,SabOut=150;
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
bool FS47Attention::SabotageHours(double Hour){return Hour>=23.5||Hour<5;}
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
FS47Attention::ESab FS47Attention::PickSab(bool bAtSaro,int32 Seed,int32 Turn,ESab Last,TFunctionRef<bool(ESab)> Can)
{
 static const ESab Here[]={ESab::Relay,ESab::Mains,ESab::Lamp},Away[]={ESab::Truck,ESab::Lamp};
 const ESab* L=bAtSaro?Here:Away;const int32 N=bAtSaro?3:2;
 ESab Ok[3];int32 Count=0;
 for(int32 I=0;I<N;++I)if(L[I]!=Last&&Can(L[I]))Ok[Count++]=L[I];
 if(Count==0)return ESab::None;
 return Ok[FMath::Min(Count-1,int32(FS47NightRhythm::Roll(Seed,FMath::Max(0,Turn),0x53414254u)*Count))];
}
bool FS47Attention::SabVariantB(int32 Seed,int32 Night){return FS47NightRhythm::Roll(Seed,Night,0x53414256u)<.5f;}
FString FS47Attention::SabScript(ESab K,bool bB){return FString(TEXT("attention_sab_"))+SabName(K)+(bB?TEXT("_b"):TEXT(""));}
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
 SabNight=S47J::Int(Saved,TEXT("sabNight"));SabCount=S47J::Int(Saved,TEXT("sabs"));Sab=SabFrom(S47J::Str(Saved,TEXT("sab")));LastSab=SabFrom(S47J::Str(Saved,TEXT("lastSab")));
 VisitNight=S47J::Int(Saved,TEXT("visitNight"));VisitCount=S47J::Int(Saved,TEXT("visits"));
 Tracked.Reset();Claims.Reset();
 for(const FString& Id:S47J::Strs(Saved,TEXT("tracked")))Tracked.Add(Id);
 for(const FString& Id:S47J::Strs(Saved,TEXT("claims")))Claims.Add(Id);
 // An older save, or one from before anything counted: what is already recorded is not counted again on the first step.
 bBaseline=S47J::Has(Saved,TEXT("score"));
 MarkNight=FeederNight=GenNight=0;GenFor=0;
 bDev=bTruckArmed=false;WarnWait=SabWait=VisitWait=StareFor=VisitT=0;ChaseDist=-1;SpareIn=-1;LearnNight=0;TellNight=0;bHintSaid=false;Visit=EVisit::None;Who=0;TakenBefore=0;
 Returns.Reset();NearMarks.Reset();ReturnNight=0;
}

void FS47Attention::Stop()
{
 // A visit under way is not kept: its sky cut and its light are the session's and FS47Taken's, which stop with it.
 Visit=EVisit::None;VisitT=0;bTruckArmed=false;SpareIn=-1;
}

S47J::FObj FS47Attention::Save() const
{
 const S47J::FObj O=S47J::New();
 if(Value<=0&&WarnCount==0&&SabCount==0&&VisitCount==0&&Sab==ESab::None)return O;
 S47J::Set(O,TEXT("score"),FMath::RoundToDouble(Value*100.)/100.);
 if(WarnCount){S47J::Set(O,TEXT("warnNight"),WarnNight);S47J::Set(O,TEXT("warns"),WarnCount);}
 if(SabCount){S47J::Set(O,TEXT("sabNight"),SabNight);S47J::Set(O,TEXT("sabs"),SabCount);}
 if(LastSab!=ESab::None)S47J::Set(O,TEXT("lastSab"),SabName(LastSab));
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
 StepTell(Hour,Night);
 StepHint(Hour,Day);
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
 // POWER-CHOICE-20261112: the standby generator is loud. Run in the dark hours it is heard out there, wherever the player is; said once a night at SARO.
 if(F->Field&&F->Field->GeneratorRunning()&&WarnHours(FMath::Fmod(FMath::Max(0.,F->Clock()),86400.)/3600.))
 {
  Add(GenPerSec*Dt,TEXT("the standby generator running in the dark"));
  GenFor+=Dt;
  const int32 GenNow=FS47FreeformSession::NightOf(F->Clock());
  if(GenNight!=GenNow&&GenFor>=GenWordAfter&&AtSaro()&&M->UI&&!M->ModalOpen())
  {
   GenNight=GenNow;
   M->UI->Thought(TEXT("That generator is loud. Out here it carries for miles, and anything out there can hear where I am."),5.f);
  }
 }
 else GenFor=0;
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
 // N1110 (critic): the first long look of a night is answered once by the gear, weakly and without a word, so the rule can be learned.
 if(LearnNight!=Night&&bStare&&(StareFor>=20||(bCamera&&StareFor>=8)))
 {
  LearnNight=Night;
  FS47OmenSource Src;Src.Id=TEXT("script:attention_learn"); // "script:" is skipped by the gains above: this is not a source to stare at
  Src.At=Feet;Src.Severity=.15;Src.Reach=FS47Omen::ReachFor(Src.Severity);
  M->Omen->Post(Src,1.5);
 }
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
  if(WarnCount==0&&!bHintSaid){bHintSaid=true;Play(S47AttentionLocal::HintRule);}
  WarnNight=Night;++WarnCount;
  return;
 }
}

// N1110 (pace): below the first warning, the night's sound dips for a second and nothing else follows. Once a night, in the warning hours.
void FS47Attention::StepTell(double Hour,int32 Night)
{
 if(Value<TellAt||Value>=WarnAt||TellNight==Night||!WarnHours(Hour))return;
 TellNight=Night;
 if(Gated(S47AttentionLocal::TellId))Play(S47AttentionLocal::TellId);
}

// N1110 (pace): Dale's word on the stare is said once on day 3 even if nothing has warned yet (it is kept as a field record when said).
void FS47Attention::StepHint(double Hour,int32 Day)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(bHintSaid||Day<HintDay||!F||!WarnHours(Hour))return;
 if(F->HasFinding(HintKey)){bHintSaid=true;return;}
 // N1110 (pace, critic 2): not over another happening, a rest, the wheel or a window, and not at the start of the night or on top of Dale.
 if(!M||!M->World||M->World->bDriving||M->ModalOpen()||M->OneAtATime())return;
 if(F->Director){FS47RhythmNow Now;F->Director->FillRhythm(Now);if(F->Director->Busy()||Now.RestLeft>0)return;}
 const double Clock=F->Clock(),IntoNight=(Hour>=18?Hour-18:Hour+6)*60.;
 const double SinceDale=M->FieldRadio.IsValid()?(Clock-M->FieldRadio->LastDaleAt())/60.:1e9;
 if(!HintQuietEnough(IntoNight,SinceDale)&&!HintForced(Day,Hour))return; // N1110 (pace, critic 3)
 if(!Gated(HintNeutral))return;
 bHintSaid=true;
 Play(HintNeutral);
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
 case ESab::Lamp:return L&&L->bHave&&L->bDrains&&L->BatteryLevel()>.3&&((F->Field&&F->Field->InventoryCount(TEXT("cell"))>0)||AtSaro()); // N1110 (critic): never with no way to put it right
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
 case ESab::Mains:return !F->SkyCutLive()&&!F->PowerFault(); // POWER-CHOICE-20261112: the lockout it leaves is still its own
 case ESab::Lamp:return !L||!L->bHave||L->BatteryLevel()>=.5;
 case ESab::Truck:return !bTruckArmed&&!(M->World&&M->World->Trouble.WantsKey());
 default:return true;
 }
}

bool FS47Attention::FeederFault() const
{
 const FS47FreeformSession* F=S47AttentionLocal::Session(M);
 return F&&FeederSabotaged(Sab,F->PowerFault(),F->SkyCutLive());
}

bool FS47Attention::DoSabotage(bool bForced,ESab Want)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||Sab!=ESab::None)return false;
 const ESab K=Want!=ESab::None?(CanSab(Want)?Want:ESab::None):PickSab(AtSaro(),F->Seed(),SabCount,LastSab,[this](ESab X){return CanSab(X);});
 bool bDone=false;
 switch(K)
 {
 case ESab::Relay:bDone=F->Field&&F->Field->TriggerServiceFault(false);break;
 case ESab::Mains:F->SkyCutBegin(SabSag,SabOut);bDone=F->SkyCutLive();break;
 case ESab::Lamp:if(FS47Flashlight* L=Lamp(M)){L->SetBattery(FMath::Min(L->BatteryLevel(),.06));bDone=true;if(F->Field&&F->Field->InventoryCount(TEXT("cell"))>0)SpareIn=6.;}break; // N1110 (critic): with a spare, a word on how
 case ESab::Truck:M->World->bKeyStall=true;bTruckArmed=true;bDone=true;break;
 default:break;
 }
 if(!bDone)return false;
 Sab=LastSab=K;++SabCount;bSabB=SabVariantB(F->Seed(),FS47FreeformSession::NightOf(F->Clock())); // N1110 (critic, sabvar)
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention %.0f: sabotage, %s%s"),Value,SabName(K),bForced?TEXT(" (F10)"):TEXT(""));
 // The truck's trace waits for the player to be at the wheel (StepSabotage).
 if(K!=ESab::Truck)Play(SabScript(K,bSabB));
 if(SabCount==1)Play(HintVisit);
 return true;
}

void FS47Attention::StepSabotage(double Dt,double Hour,int32 Night,int32 Day)
{
 // N1110 (critic): the thought about the spare cell, after the script's own lines.
 if(SpareIn>=0&&(SpareIn-=Dt)<0){SpareIn=-1;if(Sab==ESab::Lamp&&M->UI)M->UI->Thought(TEXT("I have a spare cell. Switch the lamp off, then press F."),4.5f);}
 // N1110 (critic): a truck sabotage nobody met by morning is let go on a new night, or no further sabotage could ever come.
 if(Sab==ESab::Truck&&SabNight!=0&&Night>SabNight){UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the truck's trouble from night %d is let go"),SabNight);Sab=ESab::None;bTruckArmed=false;} // (the stall itself stays: it is the truck's own trouble until the key is turned)
 // The truck that wants the key: its trace when the player is at the wheel and it does not start.
 if(Sab==ESab::Truck&&bTruckArmed&&M->World&&!M->World->bKeyStall)
 {
  bTruckArmed=false;
  if(M->World->bDriving&&M->World->Trouble.WantsKey())Play(SabScript(ESab::Truck,bSabB));
 }
 if(Sab!=ESab::None&&SabOver()){UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the %s is put right"),SabName(Sab));Sab=ESab::None;}
 StepFeeder(Night);
 if(Value<SabotageAt||Day<FirstDay||SabNight==Night||Sab!=ESab::None||!SabotageHours(Hour)){SabWait=0;return;}
 FString Why;
 if(FS47ScriptPlayer::BigEvent(M,Why))return;
 if((SabWait+=Dt)<SabotageDelay)return;
 SabWait=0;
 if(DoSabotage(false))SabNight=Night;
}

// POWER-CHOICE-20261112 (see the header): while the feeder's lockout is this sabotage's, the remote feeder is a place they have been at.
void FS47Attention::StepFeeder(int32 Night)
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||!FeederFault())return;
 // The marks, once a night, as the lights leave them anywhere: what comes to them, and when, is the lights site's and the night's.
 if(MarkNight!=Night){MarkNight=Night;ArmFeeder();}
 // The trace, the first time the player is on foot at the panel tonight.
 if(FeederNight==Night||!F->Field||!M->Web||M->Web->Area()!=TEXT("road")||!M->Player||!M->World||M->World->bDriving||M->ModalOpen())return;
 const FVector Panel=F->Field->InteractionPoint(TEXT("ff:feeder"));
 if(Panel.IsZero()||FVector2D(M->Player->Pos.X-Panel.X,M->Player->Pos.Z-Panel.Z).Size()>FeederNear)return;
 FeederNight=Night;
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the player is at the feeder they opened (night %d)"),Night);
 if(!Play(FeederTrace)&&M->UI)M->UI->Thought(TEXT("Nobody trips a feeder like that. The fuse is black from the inside."),5.f);
}

bool FS47Attention::ArmFeeder()
{
 using namespace S47AttentionLocal;
 FS47FreeformSession* F=Session(M);
 if(!F||!F->FarmSite||!F->EventPlaces||!F->Director)return false;
 // Marks standing already (somewhere else tonight, or these after a Continue) are left as they are: one site at a time.
 if((F->FarmSite->Armed()&&!F->FarmSite->Raked())||F->FarmSite->FieldVisit()||F->FarmSite->VisitWantsGround())
 {
  UE_LOG(LogS47Attention,Display,TEXT("S47: attention: marks already stand at %s; none put at the feeder"),*F->FarmSite->PlaceId());
  return false;
 }
 const FS47EventPlace* P=F->EventPlaces->Find(FeederPlace);
 const FVector At=P?F->Director->LightPlacePoint(FeederPlace):FVector::ZeroVector;
 if(!P||At.IsZero()){UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the feeder is not on the place list yet; no marks at it"));return false;}
 F->FarmSite->Arm(F->Clock(),P->Id,P->Title,FS47EventPlaces::AreaOf(*P),At);
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention: marks at the feeder they opened (%s)"),*P->Title);
 return true;
}

void FS47Attention::StepVisitDraw(double Dt,double Hour,int32 Night,int32 Day)
{
 using namespace S47AttentionLocal;
 const FS47FreeformSession* F=Session(M);
 if(!F||Value<VisitAt||!VisitMay(Day,Night,VisitNight)||!AtSaro()){VisitWait=0;return;}
 const FS47EventScript* S=S47EventScript::Find(VisitId);
 if(!VisitHours(Hour,Night,S&&S->Gates.bEarly)){VisitWait=0;return;}
 FString Why;
 if(FS47ScriptPlayer::BigEvent(M,Why)||(F->Taken&&F->Taken->Busy())||F->BigThingToday()||!Gated(VisitId))return; // N1110 (pace, critic 3): one big thing a night
 if((VisitWait+=Dt)<VisitDelay)return;
 VisitWait=0;
 // N1110 (N1110-6): inside with the power on, the visit may come as the door loop instead (S47DoorLoop.h); it is spent the same way.
 if(F->DoorLoop&&F->DoorLoop->TakeVisit())
 {
  VisitNight=Night;++VisitCount;Value=AfterVisit(EOutcome::Held,false,Value);
  UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the visit comes as the door loop, now %.0f"),Value);
  return;
 }
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

// STORY steg 6, the morning act (SC11 "SARO is searched"): the visit started by the story director on a Story night, past the score,
// the hours, the dice and the one-at-a-time hold a story script keeps on the night (OneAtATime is true while it runs). The visit itself
// is the same: BeginVisit, then StepVisit's outcome by where the player is and whether the lights are on.
bool FS47Attention::StartForStory(FString& Why)
{
 using namespace S47AttentionLocal;
 const FS47FreeformSession* F=Session(M);
 if(!FS47StoryDirector::Of(M)){Why=TEXT("not a Story night");return false;}
 if(!F||!F->Active()){Why=TEXT("no Freeform session");return false;}
 if(Visit!=EVisit::None){Why=TEXT("a visit is under way already");return false;}
 if(F->Taken&&F->Taken->Busy()){Why=TEXT("being taken");return false;}
 if(!AtSaro()){Why=TEXT("not at SARO on foot");return false;}
 BeginVisit();
 if(Visit==EVisit::None){Why=TEXT("it did not begin");return false;}
 UE_LOG(LogS47Attention,Display,TEXT("S47: attention: the visit at SARO is the story's"));
 return true;
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
 // N1110 (N1110-7): the door loop's chase, if the player ran for a door from the light, is still this visit.
 if(((F->Taken&&F->Taken->Busy())||(F->DoorLoop&&F->DoorLoop->Chasing()))&&VisitT<600)return;
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
 // POWER-CHOICE-20261112: the transformer's one, with its 20 s sag and 150 s outage before the feeder locks out and the marks go out to it.
 Line(TEXT("Attention: sabotage the mains now (feeder lockout)"),TEXT("at SARO: the sag, the mains out about three minutes, then the feeder locks out with marks at it"),[](FS47Attention& A){A.bDev=true;A.Value=FMath::Max(A.Value,55.);const bool bDone=A.DoSabotage(true,ESab::Mains);if(bDone&&A.M&&A.M->Freeform)A.SabNight=FS47FreeformSession::NightOf(A.M->Freeform->Clock());return bDone?A.Status():FString(TEXT("the mains cannot be taken now (a fault or a cut already, or one still out): "))+A.Status();});
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
 Check(Fall(VisitAt,2*86400.)>WarnAt&&Fall(Top,7*86400.)<WarnAt&&Fall(WarnAt+FallPerDay*.5,86400.)<WarnAt,TEXT("from a visit's level it still warns two days on, a quiet week is calm again, and a calm day cools a warning level below it"));
 // A look costs nothing; a long one does, more through the camera.
 const auto Look=[](double Seconds,bool bCamera){double Sum=0,Held=0;for(double T=0;T<Seconds-1e-9;T+=.05){Held+=.05;Sum+=StareGain(Held,.05,bCamera);}return Sum;};
 Check(Look(3,false)==0&&Look(1.5,true)==0,TEXT("a glance, even through the camera, adds nothing"));
 Check(FMath::IsNearlyEqual(Look(30,false),StarePerSec*(30-StareGrace),.01)&&FMath::IsNearlyEqual(Look(30,true),CameraPerSec*(30-CameraGrace),.01)&&Look(30,true)>2*Look(30,false),TEXT("half a minute held on a light adds, through the camera more than twice as much"));
 Check(ReturnGain(1)==0&&ReturnGain(2)==ReturnAgain&&ReturnGain(3)==2*ReturnAgain&&ReturnGain(6)==2*ReturnAgain,TEXT("the first walk into fresh marks is the job; going back again is not"));
 Check(PhotoGain(0)==PhotoBase&&PhotoGain(1)==PhotoBase+PhotoClose&&PhotoGain(5)==PhotoGain(1),TEXT("a photograph adds, more when it fills the print"));
 // An ordinary night of work stays calm; a night of bothering them comes to a visit.
 {
  // N1110 (pace): with the first warning at 18 a light night (one strange card, two photographs, short looks, one walk into marks) stays calm,
  // and a busy one (two cards) may meet the first warning, but never sabotage.
  const double Light=TrackRecorded+2*PhotoGain(.5)+4*Look(3,true)+Look(10,false)+ReturnGain(1);
  const double Work=Light+TrackRecorded;
  Check(Light<WarnAt&&Work<SabotageAt,FString::Printf(TEXT("a light night (one strange card, two photographs, short looks, one walk into marks) stays calm: %.1f; a busy one only warns: %.1f"),Light,Work));
  Check(TellAt>0&&TellAt<WarnAt&&WarnAt==18&&HintDay==3,TEXT("the tell comes below the first warning (12 before 18), and Dale's word is due on day 3"));
  const double Pester=TrackRecorded+2*(TrackRecorded+TrackAgain)+Look(60,true)+10*LampPerSec+2*PhotoGain(.95)+ReturnGain(2)+ReturnGain(3);
  Check(Pester>=VisitAt,FString::Printf(TEXT("a night of bothering them (the same source three times, a minute through the camera, the lamp on one, close photographs, back in the marks) comes to a visit: %.1f"),Pester));
  const double Watch=Look(90,false);
  Check(Watch>=WarnAt-10&&Watch<SabotageAt,FString::Printf(TEXT("a long watch with the eyes alone comes near the warnings, not to sabotage: %.1f"),Watch));
 }
 // The hours of each tier.
 Check(WarnHours(18.5)&&WarnHours(4)&&!WarnHours(12)&&!WarnHours(6),TEXT("warnings from 18:00 to 05:30"));
 Check(SabotageHours(23.5)&&SabotageHours(3)&&!SabotageHours(22)&&!SabotageHours(20)&&!SabotageHours(5.5),TEXT("sabotage from 23:30 to 05:00 (evenings stay deniable)"));
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
  // N1110 (critic, sabvar): drawn from the seed, not in strict turn; never the kind before; every kind reachable; the same for the same game.
  {
   int32 Seen[5]={0,0,0,0,0},Seen2[5]={0,0,0,0,0};bool bRepeat=false,bNoTruck=true,bSame=true,bAwayOnly=true,bPass=true;
   for(int32 Seed=1;Seed<=40;++Seed)
   {
    ESab LastH=ESab::None,LastA=ESab::None;
    for(int32 T=0;T<30;++T)
    {
     const ESab H=PickSab(true,Seed,T,LastH,All),A=PickSab(false,Seed,T,LastA,All);
     bRepeat|=H==LastH||A==LastA;bNoTruck&=H!=ESab::Truck&&H!=ESab::None;bAwayOnly&=(A==ESab::Truck||A==ESab::Lamp);bSame&=H==PickSab(true,Seed,T,LastH,All);
     ++Seen[int32(H)];++Seen2[int32(A)];LastH=H;LastA=A;
     bPass&=PickSab(true,Seed,T,H,NoRelay)!=ESab::Relay&&PickSab(true,Seed,T,H,NoRelay)!=H;
    }
   }
   Check(!bRepeat&&bNoTruck&&bAwayOnly&&bSame&&bPass,TEXT("N1110 (sabvar): over 40 games of 30 sabotages the kind never repeats the one before, the truck is never the station's, and the same game draws the same"));
   Check(Seen[int32(ESab::Relay)]>100&&Seen[int32(ESab::Mains)]>100&&Seen[int32(ESab::Lamp)]>100&&Seen2[int32(ESab::Truck)]>100&&Seen2[int32(ESab::Lamp)]>100,TEXT("N1110 (sabvar): every kind comes up (at SARO the relay, the mains and the lamp; away the truck and the lamp), none starved"));
   bool bOnly=PickSab(false,3,0,ESab::Truck,All)==ESab::Lamp&&PickSab(false,3,0,ESab::Lamp,All)==ESab::Truck&&PickSab(false,3,0,ESab::Lamp,[](ESab K){return K==ESab::Lamp;})==ESab::None&&PickSab(true,3,0,ESab::None,Nothing)==ESab::None&&PickSab(true,3,0,ESab::Mains,[](ESab K){return K==ESab::Mains;})==ESab::None;
   Check(bOnly&&PickSab(true,3,0,ESab::None,NoRelay)!=ESab::Relay,TEXT("N1110 (sabvar): what cannot be done is passed over, the one before is not drawn again even if it is all that can be done, and nothing when nothing can"));
   int32 B=0;for(int32 N=1;N<=200;++N)B+=SabVariantB(7,N);
   Check(B>60&&B<140&&SabVariantB(7,5)==SabVariantB(7,5)&&SabScript(ESab::Relay,false)==TEXT("attention_sab_relay")&&SabScript(ESab::Truck,true)==TEXT("attention_sab_truck_b"),FString::Printf(TEXT("N1110 (sabvar): the wording is drawn per night, both about half the time (%d of 200 on b), and named attention_sab_<kind>[_b]"),B));
  }
  // POWER-CHOICE-20261112: the transformer's fault, and the generator's noise against the county crew's hours.
  Check(FeederSabotaged(ESab::Mains,true,false)&&!FeederSabotaged(ESab::Mains,true,true)&&!FeederSabotaged(ESab::Mains,false,false)&&!FeederSabotaged(ESab::Relay,true,false)&&!FeederSabotaged(ESab::None,true,false),
   TEXT("POWER-CHOICE: a feeder fault is the sabotage's only once the mains it took have locked out, and only the mains'"));
  {
   const double DarkHour=45.*60./11.; // seconds of play in a game hour of the dark (11 hours in 45 minutes)
   const double ToCrew=GenPerSec*DarkHour*FS47FreeformSession::SkyFaultMinutes/60.,AllNight=GenPerSec*DarkHour*11.;
   Check(ToCrew>=10&&SabotageAt+ToCrew<VisitAt&&SabotageAt+AllNight>=VisitAt&&WarnHours(23.5)&&WarnHours(3)&&!WarnHours(12),
    FString::Printf(TEXT("POWER-CHOICE: the generator is heard in the dark: run until the county crew comes it adds %.1f (from the sabotage's level near a visit, not to one by itself); a whole night of it, %.1f, brings one"),ToCrew,AllNight));
  }
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
  Ids.Append({S47AttentionLocal::TellId,S47AttentionLocal::HintRule,S47AttentionLocal::HintNeutral,S47AttentionLocal::HintVisit,S47AttentionLocal::VisitId,S47AttentionLocal::VisitHeld,S47AttentionLocal::VisitEscaped});
  for(ESab K:{ESab::Relay,ESab::Mains,ESab::Lamp,ESab::Truck}){Ids.Add(SabScript(K,false));Ids.Add(SabScript(K,true));} // N1110 (sabvar): both wordings
  Ids.Add(S47AttentionLocal::FeederTrace); // POWER-CHOICE-20261112
  int32 Found=0;bool bClean=true;FString First;
  for(const FString& Id:Ids)
  {
   const FS47EventScript* S=S47EventScript::Find(Id);if(!S)continue;
   ++Found;const TArray<FString> E=S47EventScript::Lint(*S);
   if(E.Num()){bClean=false;if(First.IsEmpty())First=Id+TEXT(": ")+E[0];}
  }
  if(Found==0)Lines.Add(TEXT("NOTE attention: no attention_* event scripts beside this build: plain lines play instead"));
  else Check(bClean&&Found==Ids.Num(),FString::Printf(TEXT("all %d of its event scripts are there and clean%s"),Ids.Num(),First.IsEmpty()?TEXT(""):*(TEXT(": ")+First)));
  // N1110 (critic): the mains' tubes come when they return; Dale's word 10 s after the warning's; the dog's quiet; the lights over SARO only.
  {
   const FS47EventScript* Mn=S47EventScript::Find(TEXT("attention_sab_mains"));
   const FS47EventScript* Hn=S47EventScript::Find(S47AttentionLocal::HintRule);
   const FS47EventScript* Dg=S47EventScript::Find(TEXT("attention_warn_dog"));
   const FS47EventScript* Li=S47EventScript::Find(TEXT("attention_warn_lights"));
   if(Mn)
   {
    bool bAfter=true;int32 Tubes=0;
    for(const FS47Beat& B:Mn->Beats)if(B.Do==ES47Beat::Tube){++Tubes;bAfter&=B.At>S47AttentionLocal::SabSag+S47AttentionLocal::SabOut;}
    Check(Tubes==1&&bAfter,TEXT("the mains' tube beat comes after the outage, not in the dark"));
   }
   {
    // N1110 (attfix): every sabotage that darkens the ceiling points at the shop counter that sells the tubes back, and the lowering rule is said.
    bool bShop=true;int32 Sabs=0;
    TSet<FString> Words;
    for(const TCHAR* Id:{TEXT("attention_sab_relay"),TEXT("attention_sab_lamp"),TEXT("attention_sab_truck"),TEXT("attention_sab_mains"),TEXT("attention_sab_relay_b"),TEXT("attention_sab_lamp_b"),TEXT("attention_sab_truck_b"),TEXT("attention_sab_mains_b")})
     if(const FS47EventScript* Sb=S47EventScript::Find(Id)){++Sabs;bShop&=Sb->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Thought&&B.Text.Contains(TEXT("shop counter"))&&B.Text.Contains(TEXT("pair"));});for(const FS47Beat& B:Sb->Beats)if(B.Do==ES47Beat::Thought)Words.Add(B.Text);}
    if(Sabs==8)Check(Words.Num()==16,TEXT("N1110 (sabvar): the eight sabotage scripts (two wordings of four kinds) have sixteen different thoughts"));
    const FS47EventScript* Hv=S47EventScript::Find(S47AttentionLocal::HintVisit);
    if(Sabs>0)Check(bShop,TEXT("N1110 (attfix): each sabotage's thought names the shop counter that sells the tubes in pairs"));
    if(Hv)Check(Hv->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Line&&B.Text.Contains(TEXT("lose interest"));}),TEXT("N1110 (attfix): Dale says how it is lowered: leave the lights alone a day or two"));
   }
   {
    const FS47EventScript* Nt=S47EventScript::Find(S47AttentionLocal::HintNeutral);
    if(Nt)Check(Nt->Beats.Num()==4&&Nt->Beats[0].Do==ES47Beat::Line&&!Nt->Beats[0].Text.Contains(TEXT("light"))&&Nt->Beats[2].At-Nt->Beats[0].At>=16&&Nt->Beats[2].Text.Contains(TEXT("hold still")),TEXT("N1110 (pace, critic 2): the neutral hint names no lights, and the lamp line comes 16 s after it"));
    Check(!HintForced(2,3)&&!HintForced(3,1.9)&&!HintForced(3,20)&&HintForced(3,2)&&HintForced(3,5)&&!HintForced(3,5.6)&&HintForced(4,19),TEXT("N1110 (pace, critic 3): the day-3 hint cannot be skipped: after 02:00 on day 3, and at the first quiet moment from day 4"));
    Check(!HintQuietEnough(9,99)&&!HintQuietEnough(99,9)&&HintQuietEnough(10,10),TEXT("N1110 (pace, critic 2): the hint waits ten game minutes after the night's start and after Dale's last line"));
   }
   // POWER-CHOICE-20261112: at the feeder the night goes still before anything is said, the trace is kept, and no tube is spent there.
   if(const FS47EventScript* Fd=S47EventScript::Find(S47AttentionLocal::FeederTrace))
    Check(Fd->Beats.Num()>0&&Fd->Beats[0].Do==ES47Beat::Silence&&Fd->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Evidence&&B.Key.StartsWith(TEXT("attention_feeder_d"));})&&!Fd->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Tube||B.Do==ES47Beat::CameraSnap;}),
     TEXT("POWER-CHOICE: the feeder's trace starts in silence, keeps its record, and spends no tube and snaps no camera"));
   if(Hn)Check(Hn->Beats.Num()>0&&Hn->Beats[0].At>=10,TEXT("Dale's hint comes 10 s or more after the warning starts"));
   if(Hn)Check(Hn->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Line&&B.Text.Contains(TEXT("hold still"));}),TEXT("N1110 (pace): Dale's word gives the rule once: kill the lamp and hold still"));
   if(Dg)Check(Dg->Beats.ContainsByPredicate([](const FS47Beat& B){return B.Do==ES47Beat::Silence;}),TEXT("the dog warning goes quiet"));
   const FS47EventScript* Tl=S47EventScript::Find(S47AttentionLocal::TellId);
   if(Tl)Check(Tl->Beats.Num()==2&&Tl->Beats[0].Do==ES47Beat::Silence&&Tl->Beats[0].Secs<=1.5&&Tl->Beats[1].Do==ES47Beat::Sound&&Tl->Beats[1].Name.StartsWith(TEXT("dog"))&&Tl->Beats[1].At>=Tl->Beats[0].Secs&&Tl->Beats[1].Gain<=.06&&Tl->Beats[1].Stop>0&&Tl->Beats[1].Stop<=.8,TEXT("the tell is one second of the night's sound dipping, then a very faint far dog for under a second (N1110 pace, critic 3), and nothing else"));
   if(Li)Check(Li->Gates.Areas.Contains(TEXT("saro")),TEXT("the lights over the station are SARO's only"));
  }
  const FS47EventScript* V=S47EventScript::Find(S47AttentionLocal::VisitId);
  if(V)Check(V->Gates.bTier&&V->Gates.Tier==ES47Allowed::Full&&!V->Gates.bEarly,TEXT("the visit's script keeps to the full tier (no early escalation as shipped)"));
 }
 return OK;
}
