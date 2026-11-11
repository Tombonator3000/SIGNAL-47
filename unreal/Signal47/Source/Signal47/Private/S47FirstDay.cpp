#include "S47Happenings.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47OfficeTv.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Predecessor.h"
#include "S47SmallHints.h"
#include "S47TvProgramme.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FirstDay,Log,All);

// D46-20261110 (D46-3, Tom 10 November: «Natt 1: må ha litt mer action/hendelser første døgnet ... Må lage ett solid første døgnet
// script for å fenge spiller»). The first day and night as a script (scratchpad d46/FIRSTDAY.md), on top of what D45-26 put in (Dale's
// hello and his bench, the noon slip toward the night's line, the dusk orb at 18:30, the nine o'clock appointment, the burst between
// 22:00 and 23:15 and the unease beside it). Game clock, and about where an unheld clock has the player in real minutes:
//   13:00-17:45  (12-21) at the wheel out on the land: a whistle on the CB, and a stranger who heard it too and blames the dishes
//   16:30-18:10  (19-23) if the whistle did not come: Dale's set in OPS tears and goes to snow, or with it off the phone rings twice
//   20:06-20:51  (30-33) at SARO: a dog far off toward the night's line barks a while, and then the crickets stop
//   the burst    (38-43) Dale's set goes to snow with it (S47Happenings.cpp, array_burst)
//   00:24-00:39  (46-48) at SARO: three clicks from the rack, or an antenna creaking in the yard
//   00:41        (49)    every dish on remote turns together to the night's line, four degrees up, and the printer runs one line
//   01:25-02:10  (52-55) in the building: the phone rings three times and stops
// Before midnight each can be explained away; after it, machinery and sound only (the glimpse tier, nothing alive). Nothing here is a
// Director event: no budget, no finding, no pointing-out, except the turn's own record.
namespace S47Hap
{
 FString ClockText(double Clock);
 bool Clean(const FString& Text);
}
namespace
{
 FString FdClock(double Clock){FString T=S47Hap::ClockText(Clock);if(T.EndsWith(TEXT(":47")))T=T.LeftChop(2)+TEXT("48");return T;}
 int32 FdIndex(const TCHAR* Id){for(int32 I=0;I<FS47Happenings::UneaseCount();++I)if(FCString::Strcmp(FS47Happenings::UneaseId(I),Id)==0)return I;return -1;}
 const TCHAR* const FdWhistle=TEXT("Breaker one-nine. Anybody else catch that whistle on the channel just now? Loud as a kettle, and then gone. Must be those big dishes acting up again.");
}

const TArray<FS47Happenings::FFirstBeat>& FS47Happenings::FirstBeats()
{
 static const TArray<FFirstBeat> Table={
  {TEXT("cb_whistle"),13.,17.75,true},
  {TEXT("office_set"),16.5,18.17,true},
  {TEXT("dog_off"),20.1,20.85,true},
  {TEXT("brakes"),24.4,24.65,false},
  {TEXT("array_turn"),FirstTurnHour,FirstTurnLive,false},
  {TEXT("phone_nobody"),25.42,26.17,false},
 };
 return Table;
}

double FS47Happenings::FirstRealMinutes(double Hour)
{
 // S47Freeform.cpp's clock: 06 to 19 is thirteen hours in thirty real minutes, 19 to 06 eleven hours in forty-five.
 double Real=0;
 for(double H=8.;H<Hour-1e-9;H+=1./60.)
 {
  const double Of=FMath::Fmod(H,24.),Step=FMath::Min(1./60.,Hour-H);
  Real+=Step*(Of>=6.&&Of<19.?30./13.:45./11.);
 }
 return Real;
}

FS47Happenings::EFirstTurn FS47Happenings::FirstTurnFate(double Hour,bool bAtSaro,bool bPowered,bool bMissed,bool bBusy)
{
 if(!FMath::IsFinite(Hour)||Hour<FirstTurnHour)return EFirstTurn::Wait;
 if(Hour>=FirstFoundUntil)return EFirstTurn::Never;
 // The dishes move only with power, and only where the player can see them or find them.
 if(!bAtSaro||!bPowered)return EFirstTurn::Wait;
 if(bMissed||Hour>=FirstTurnLive)return EFirstTurn::Found;
 return bBusy?EFirstTurn::Wait:EFirstTurn::Live;
}

double FS47Happenings::FirstBearing() const
{
 // The burst's own bearing when it has come; else the same line it would have had (FS47Places::ArcBearing, never 47 in it).
 for(const FSite& S:Sites)if(S.Kind==TEXT("burst")&&S.Night==1&&S.Az>=0)return S.Az;
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Places)return -1;
 const FString Arc=FS47Places::ArcPlace(F->Seed());double ToX=0,ToZ=0;
 if(Arc.IsEmpty()||!F->Places->Spot(Arc,ToX,ToZ))return -1;
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 return FS47Places::ArcBearing(F->Seed(),SaroX,SaroZ,ToX,ToZ,18.);
}

void FS47Happenings::EndFirstDay()
{
 if(FirstDay.bMotors&&IsValid(M)&&IsValid(M->Audio)&&!IsEngineExitRequested())M->Audio->StopMotors();
 FirstDay.bMotors=false;FirstDay.Later.Reset();
}

void FS47Happenings::StepFirstDay()
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->RulesLive()||F->InTutorial()||!M->Web)return;
 const double Hour=F->Clock()/3600.;
 // What a beat does a few seconds on.
 for(int32 I=0;I<FirstDay.Later.Num();)
 {
  if(Paced<FirstDay.Later[I].Key){++I;continue;}
  TFunction<void()> Fn=MoveTemp(FirstDay.Later[I].Value);FirstDay.Later.RemoveAt(I);
  if(Fn)Fn();
 }
 if(FirstDay.bMotors&&Paced-FirstDay.MotorsAt>3&&!M->Web->DishesMoving()){if(M->Audio)M->Audio->StopMotors();FirstDay.bMotors=false;}
 if(Hour>=FirstFoundUntil+1.)return;
 // The clock went on a long way at once (a rest, a wait, a trip): what it passed was slept through.
 const bool bJumped=FirstDay.LastHour>=0&&Hour-FirstDay.LastHour>.5;
 FirstDay.LastHour=Hour;
 if(!FirstDay.bPrimed)
 {
  FirstDay.bPrimed=true;
  if(FirstDay.bLoaded)for(const FFirstBeat& B:FirstBeats())if(Hour>=B.From&&FCString::Strcmp(B.Id,TEXT("array_turn"))!=0)FirstDay.Done.Add(B.Id);
 }
 const bool bBusy=M->OneAtATime()||!Now.Id.IsEmpty()||Uneasy.I>=0||BigLive();
 // The cliffhanger: its record is its once-flag.
 if(!F->HasFinding(FirstTurnKey)&&!FirstDay.Done.Contains(TEXT("array_turn")))
 {
  const bool bAtSaro=M->Web->Area()==TEXT("saro")&&M->Web->IsLoaded()&&M->Web->DishCount()>0;
  if(Hour>=FirstTurnHour&&(!bAtSaro||bJumped))FirstDay.bMissed=true;
  const EFirstTurn Fate=BigOwns(1)?EFirstTurn::Never:FirstTurnFate(Hour,bAtSaro,F->HasPowerForReactiveLight(),FirstDay.bMissed,bBusy);
  if(Fate==EFirstTurn::Never){FirstDay.Done.Add(TEXT("array_turn"));UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: the array's turn was never looked at by the evening after"));}
  else if(Fate!=EFirstTurn::Wait)TurnFirstArray(Fate==EFirstTurn::Found);
 }
 if(bBusy||bJumped)return;
 for(const FFirstBeat& B:FirstBeats())
 {
  const FString Id=B.Id;
  if(Id==TEXT("array_turn")||FirstDay.Done.Contains(Id)||Hour<B.From)continue;
  if(Hour>=B.Until){FirstDay.Done.Add(Id);UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: %s not met (window over)"),*Id);continue;}
  if(FirstBeat(Id)){FirstDay.Done.Add(Id);UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: %s at %s"),*Id,*S47Hap::ClockText(F->Clock()));break;}
 }
}

bool FS47Happenings::FirstBeat(const FString& Id)
{
 auto* F=M->Freeform.Get();
 US47Audio* A=M->Audio.Get();
 if(!A)return false;
 const bool bAtSaro=M->Web->Area()==TEXT("saro");
 const bool bInside=FS47FieldRadio::InSaroBuilding(M);
 const TWeakPtr<uint8> Guard=Alive;
 if(Id==TEXT("cb_whistle"))
 {
  // At the wheel out on the land, the CB under the dash quiet: a whistle, and a stranger who heard it too.
  if(!Driving()||bAtSaro||!CanHearCb()||(M->CB.IsValid()&&M->CB->bBusy)||FS47FieldRadio::DaleOnAir(M))return false;
  A->Beep(1850.f,1.6f,.03f,0.f,TEXT("sine"));A->Beep(1790.f,.7f,.02f,1.6f,TEXT("sine"));
  FirstDay.Later.Emplace(Paced+3.2,[this,Guard]{if(Guard.IsValid()&&CanHearCb())Cb({FString(FdWhistle)},TEXT("old"),false);});
  FirstDay.Done.Add(TEXT("office_set"));
  return true;
 }
 if(Id==TEXT("office_set"))
 {
  // In the building in the late afternoon: Dale's set breaks up and comes back; switched off, the phone rings twice and stops.
  if(!bInside||M->World->bDriving)return false;
  const FS47OfficeTv* Tv=M->Office?M->Office->Television():nullptr;
  const bool bSet=Tv&&!Tv->State().StartsWith(TEXT("off"));
  if(!bSet&&F->Predecessor&&F->Predecessor->Ringing())return false;
  FirstDay.Done.Add(TEXT("cb_whistle"));
  if(bSet){FS47TvProgramme::JamFor(.85,3.5);S47SmallHints::Play(M,TEXT("rack_click"),S47SmallHints::AnchorOf(M,TEXT("rack_click")));return true;}
  const FVector Phone=S47SmallHints::AnchorOf(M,TEXT("phone_one_ring"));
  S47SmallHints::Play(M,TEXT("phone_one_ring"),Phone);
  FirstDay.Later.Emplace(Paced+3.,[this,Guard,Phone]{if(Guard.IsValid()&&M)S47SmallHints::Play(M,TEXT("phone_one_ring"),Phone);});
  return true;
 }
 if(Id==TEXT("dog_off"))
 {
  // At SARO after dark: a dog a long way off, out the way the night's line runs, and then the crickets go quiet.
  if(!bAtSaro||M->World->bDriving)return false;
  // (toward the night's line where it lies on the land, as the big night places its dog; else off to one side)
  FVector Way(1,0,0);
  double ToX=0,ToZ=0;FVector Ground;
  if(F->Places&&F->Places->Spot(FS47Places::ArcPlace(F->Seed()),ToX,ToZ)&&WorldOf(ToX,ToZ,Ground)){FVector D=Ground-Eye();D.Y=0;if(D.Normalize())Way=D;}
  A->Far(TEXT("dog1"),Eye()+Way*340.+FVector(0,2,0),bInside?.16f:.32f,.9f);
  FirstDay.Later.Emplace(Paced+5.5,[this,Guard]{if(Guard.IsValid()&&M&&Outdoors())S47SmallHints::Play(M,TEXT("night_hush"),FVector::ZeroVector);});
  return true;
 }
 if(Id==TEXT("brakes"))
 {
  // The array about to move: relays in the rack, or out in the yard an antenna giving.
  if(!bAtSaro||M->World->bDriving||F->HasFinding(FirstTurnKey))return false;
  const TCHAR* Hint=bInside?TEXT("rack_click"):TEXT("antenna_creak");
  S47SmallHints::Play(M,Hint,S47SmallHints::AnchorOf(M,Hint));
  return true;
 }
 if(Id==TEXT("phone_nobody"))
 {
  // Half past one: the phone, three rings, and it stops before anyone could get to it. Roof ticks if tonight's unease was the phone.
  if(!bInside||M->World->bDriving)return false;
  if(F->Predecessor&&F->Predecessor->Ringing())return false;
  const int32 PhoneUnease=FdIndex(TEXT("phone_breath"));
  if(PhoneUnease>=0&&UneaseDone.Contains(PhoneUnease)){S47SmallHints::Play(M,TEXT("roof_tick"),S47SmallHints::AnchorOf(M,TEXT("roof_tick")));return true;}
  const FVector Phone=S47SmallHints::AnchorOf(M,TEXT("phone_one_ring"));
  S47SmallHints::Play(M,TEXT("phone_one_ring"),Phone);
  for(int32 Ring=1;Ring<3;++Ring)FirstDay.Later.Emplace(Paced+3.*Ring,[this,Guard,Phone]{if(Guard.IsValid()&&M)S47SmallHints::Play(M,TEXT("phone_one_ring"),Phone);});
  return true;
 }
 return false;
}

void FS47Happenings::TurnFirstArray(bool bFound)
{
 auto* F=M->Freeform.Get();
 AS47Web* W=M->Web.Get();
 FirstDay.Done.Add(TEXT("array_turn"));
 const double Az=FirstBearing(),El=4.;
 if(Az<0||!W){UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: no line for the array to turn to"));return;}
 int32 Turned=0;
 for(int32 I=0;I<W->DishCount();++I)
 {
  if(!W->DishRemoteAvailable(I))continue;
  if(bFound)W->SnapDish(I,Az,El);
  W->PointDish(I,Az,El,bFound?9.:3.5);++Turned;
 }
 if(Turned==0){UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: no dish on remote to turn"));return;}
 const FString At=FdClock(bFound?FirstTurnHour*3600.:F->Clock());
 const FString Strip=FString::Printf(TEXT("%s  ARRAY ON REMOTE  SLEW %03.0f  EL %.0f  OPERATOR: NONE"),*At,Az,El);
 const bool bBurst=SiteOf(TEXT("burst"))!=nullptr;
 if(!bFound)
 {
  if(M->Audio&&!FirstDay.bMotors){M->Audio->StartMotors();FirstDay.bMotors=true;FirstDay.MotorsAt=Paced;}
  // Dale's set breaks up while they move; then the printer, one line by itself.
  FS47TvProgramme::JamFor(.6,9.);
  if(M->Audio){FS47Sound P;P.At=FVector(-4,1,2);P.Gain=.3f;P.When=6.f;const FS47Play Line=M->Audio->Sfx(TEXT("printer"),P);if(Line&&M->Audio->Ctx.IsValid())Line.Src->Stop(M->Audio->Ctx->CurrentTime()+8.5);}
  Say(FString::Printf(TEXT("ARRAY / The dish motors are running. Every antenna on remote is turning, all together, to about %03.0f and %.0f up. Nobody is at the console."),Az,El),.5,7);
  Say(FString::Printf(TEXT("PRINTER / One line by itself: %s"),*Strip),7.5,7);
 }
 else if(M->UI)M->UI->Toast(FString::Printf(TEXT("ARRAY / Every antenna on remote is pointed at %03.0f, %.0f up, all the same way. Nobody commanded it. On the printer's strip, one line by itself: %s"),Az,El,*Strip),9);
 F->RecordEvidence(FirstTurnKey,FString::Printf(TEXT("At %s every antenna on remote turned by itself, all together, to about %03.0f degrees and %.0f up%s, and stayed there. Nothing on the command log. The printer ran one line by itself: %s.%s"),
  *At,Az,El,bBurst?TEXT(", the same bearing as the burst off the main beam"):TEXT(""),*Strip,bFound?TEXT(" I found it that way; I did not see it move."):TEXT("")));
 UE_LOG(LogS47FirstDay,Display,TEXT("S47: first day: %d dishes on remote %s %.0f, %.0f up"),Turned,bFound?TEXT("found at"):TEXT("turn to"),Az,El);
 M->RequestAutosave();
}

bool FS47Happenings::FirstDaySelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s first day: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 const TArray<FFirstBeat>& Beats=FirstBeats();
 {
  bool bOrder=true,bTier=true,bClock=true;TSet<FString> Seen;double Last=0;
  for(const FFirstBeat& B:Beats)
  {
   bOrder&=B.From<B.Until&&B.From>=8.&&B.Until<=32.&&B.From>=Last&&!Seen.Contains(B.Id);Last=B.From;Seen.Add(B.Id);
   // Before midnight only what can be explained away; from midnight to two machinery and sound (no beat here is a body).
   bTier&=(B.From<24.)==B.bHint&&B.Until<=26.33;
   for(const double H:{B.From,B.Until}){const FString T=S47Hap::ClockText(H*3600.);bClock&=!T.EndsWith(TEXT(":47"))&&T!=TEXT("08:15")&&T!=TEXT("20:15");}
  }
  Check(bOrder&&Seen.Num()==Beats.Num(),TEXT("the script's beats are in order, each once, each window inside night 1"));
  Check(bTier,TEXT("before midnight only hints; nothing past two, clear of Story's 02:20 to 02:40"));
  Check(bClock,TEXT("no beat opens or closes at a :47, at 08:15 or 20:15"));
  Check(S47Hap::ClockText(FirstTurnHour*3600.)==TEXT("00:41")&&FirstTurnHour>FirstBurstFrom+FirstBurstSpan&&FirstTurnLive<=FirstBurstUntil&&FirstFoundUntil==42.,
   TEXT("the array turns at 00:41, after the burst's window; live until 01:30, found until 18:00 on day 2"));
 }
 {
  // From the noon slip to the TV's sign-off nothing leaves a player who keeps to the building more than twelve real minutes with
  // nothing (an unheld clock): D45-26's fixed beats (his hello, the slip, the orbs, the appointment, the burst at the latest of its
  // window, the sign-off) and these as they open; the whistle needs the truck, so the set in OPS stands for the afternoon.
  TArray<double> At={10.5,12.47,18.5,19.,21.25,FirstBurstFrom+FirstBurstSpan,26.};
  for(const FFirstBeat& B:Beats)if(FCString::Strcmp(B.Id,TEXT("cb_whistle"))!=0)At.Add(B.From);
  At.Sort();double Gap=0;
  for(int32 I=1;I<At.Num();++I)Gap=FMath::Max(Gap,FirstRealMinutes(At[I])-FirstRealMinutes(At[I-1]));
  Check(Gap<=12.,FString::Printf(TEXT("the first day has something every twelve real minutes or less from the noon slip to the sign-off (longest %.1f)"),Gap));
  Check(FMath::Abs(FirstRealMinutes(18.5)-24.2)<.3&&FMath::Abs(FirstRealMinutes(FirstTurnHour)-48.7)<.4&&FirstRealMinutes(8.)==0,TEXT("real minutes as DAY1_now has them: the dusk orb at 24, the turn at 49"));
 }
 {
  const double T=FirstTurnHour+.01;
  Check(FirstTurnFate(FirstTurnHour-.01,true,true,false,false)==EFirstTurn::Wait&&FirstTurnFate(T,true,true,false,false)==EFirstTurn::Live
   &&FirstTurnFate(T,true,true,false,true)==EFirstTurn::Wait&&FirstTurnFate(T,true,true,true,false)==EFirstTurn::Found
   &&FirstTurnFate(T,false,true,true,false)==EFirstTurn::Wait&&FirstTurnFate(T,true,false,false,false)==EFirstTurn::Wait
   &&FirstTurnFate(FirstTurnLive,true,true,false,false)==EFirstTurn::Found&&FirstTurnFate(31.,true,true,true,false)==EFirstTurn::Found
   &&FirstTurnFate(FirstFoundUntil,true,true,true,false)==EFirstTurn::Never&&FirstTurnFate(FirstTurnHour,true,true,false,false)==EFirstTurn::Live,
   TEXT("the turn: live at SARO awake, waits while something else holds the ground or without power, found when slept through or away, never after the evening after"));
 }
 Check(S47Hap::Clean(FdWhistle)&&!FString(FdWhistle).Contains(TEXT("47"))&&FString(FdWhistle).Len()<190,TEXT("the stranger's words name nobody and fit the CB line"));
 return OK;
}
